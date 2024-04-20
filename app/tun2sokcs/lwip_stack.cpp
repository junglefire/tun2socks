#include "conn_session.h"
#include "lwip_proxy.h"
#include "lwip_stack.h"
#include "lwip_echo.h"
#include "nat_table.h"

LWIPStack::LWIPStack() {
	this->tpcb = NULL;
	this->upcb = NULL;
}

LWIPStack::~LWIPStack() {
	if (this->tpcb != NULL) {
		memp_free(MEMP_TCP_PCB, this->tpcb);
	}
	if (this->upcb != NULL) {
		memp_free(MEMP_UDP_PCB, this->upcb);
	}
}

int LWIPStack::init(const char* service_type) {
	// Initialize lwIP.
	lwip_init();

	// setup MTU
	netif_list->mtu = 1500;
	
	// init tcp module
	this->tpcb = tcp_new();
	if (this->tpcb == NULL) {
		error("tcp_new() failed, abort!");
		return -1;
	}
	
	err_t err = tcp_bind(this->tpcb, IP_ADDR_ANY, 0);
	if (err != ERR_OK) {
		error("tcp_bind() failed, errno: %d", err);
		return -1;
	}
	
	// 第一个参数`this->tpcb`会被释放，返回一个新的tpcb
	this->tpcb = tcp_listen_with_backlog(this->tpcb, TCP_DEFAULT_LISTEN_BACKLOG);
	if(this->tpcb == NULL) {
		error("tcp_listen_with_backlog() failed, abort!\n");
		return -1;
	}

	info("allocate tcp pcb ok");

	//init udp module
	this->upcb = udp_new();
	if(this->upcb == NULL) {
		error("udp_new() failed, abort!");
		return -1;
	}
	
	err = udp_bind(this->upcb, IP_ADDR_ANY, 0);
	if(err != ERR_OK) {
		error("udp_bind() failed, errno: %d", err);
		return -1;
	}

	info("allocate udp pcb ok");

	if (std::strcmp(service_type, "proxy") == 0) {
		info("register lwip proxy callback...");
		tcp_accept(this->tpcb, LWIPStack::lwip_proxy_tcp_accept_cb);
		udp_recv(this->upcb, LWIPStack::lwip_proxy_udp_recv_cb, NULL);
	} else if (std::strcmp(service_type, "echo") == 0) {
		info("register lwip echo callback...");
		tcp_accept(this->tpcb, LWIPStack::lwip_echo_tcp_accept_cb);
		udp_recv(this->upcb, LWIPStack::lwip_echo_udp_recv_cb, NULL);
		// tcp_accept(this->tpcb, LWIPEchoCallback::tcp_accept_cb);
		// udp_recv(this->upcb, LWIPEchoCallback::udp_recv_cb, NULL);
	} else {
		error("invalid service type, abort!");
		return -1;
	}
	return 0;
}

// LWIPEcho的工厂方法，接收新的连接，创建新的lwip的句柄
// 当client与lwip stack完成三次握手之后回调函数
err_t LWIPStack::lwip_echo_tcp_accept_cb(void* arg, struct tcp_pcb* newpcb, err_t err) {
	info("create `LWIPEcho` instance...");

	if(err != ERR_OK || (newpcb == NULL)) {
		error("LWIPStack::tcp_accept_cb() failed, errno: %d", err);
		return ERR_VAL; //Illegal value;
	}

	LWIP_UNUSED_ARG(arg);

	// Unless this pcb should have NORMAL priority, set its priority now.
	// When running out of pcbs, low priority pcbs can be aborted to create
	// new pcbs of higher priority. 
	tcp_setprio(newpcb, TCP_PRIO_MIN);

	info("LWIPStack::tcp_accept_cb(): new connection `%s:%d`-->`%s:%d`, &tcp_pcb: %p", 
		LOCAL_IP(newpcb), LOCAL_PORT(newpcb), REMOTE_IP(newpcb), REMOTE_PORT(newpcb), newpcb);

	err_t ret;
	
	// 创建Echo对象
	LWIPEcho* echo = new LWIPEcho();
	echo->raw_api_status = raw_api_status_e::ACCEPTED;
	echo->newpcb = newpcb;
	echo->retries = 0;
	echo->pbuf_ptr = nullptr;

	// 0. 将`echo`变量作为回调函数的参数`arg`
	tcp_arg(newpcb, echo);

	// 1. 设置接收回调函数
	tcp_recv(newpcb, [](void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err)->err_t {
		// 未知错误，退出
		if(err != ERR_OK) { 
			error("cleanup, for unknown reason");
			LWIP_ASSERT("no pbuf expected here", p == NULL);
			return err;
		}
		LWIPEcho* echo = (LWIPEcho*)arg;
		return echo->tcp_recv_cb(p);
	});

	// 2. 设置错误处理回调，因为pcb结构可能已经被删除了，所以在处理错误的回调函数中pcb参数不可能传递进来
	tcp_err(newpcb, [](void *arg, err_t err){
		LWIPEcho* echo = (LWIPEcho*)arg;
		return echo->tcp_error_cb(err);
	});
	
	// 3. 设置LWIP定时器
	tcp_poll(newpcb, [](void *arg, struct tcp_pcb *tpcb) {
		LWIPEcho* echo = (LWIPEcho*)arg;
		return echo->tcp_poll_cb();
	}, 0);
	
	// 4. 设置发送完成回调
	tcp_sent(newpcb, [](void* arg, struct tcp_pcb* tpcb, u16_t len) {
		LWIPEcho* echo = (LWIPEcho*)arg;
		return echo->tcp_sent_cb(len);	
	});
	
	return ERR_OK;
}

void LWIPStack::lwip_echo_udp_recv_cb(void *arg, struct udp_pcb *newpcb, struct pbuf *p, const ip_addr_t *addr, u16_t port, const ip_addr_t *dest_addr, u16_t dest_port) {
	do {
		if (newpcb == NULL) {
			break;
		}
		info("send udp packet from: `%s:%d`-->`%s:%d`", ipaddr_ntoa(addr), port, ipaddr_ntoa(dest_addr), dest_port);
		char buffer[MTU_USER_DEFINE];
		if(pbuf_copy_partial(p, buffer, p->tot_len,0) != p->tot_len) {
			error("pbuf_copy_partial() failed");
		} else {
			buffer[p->tot_len] = '\0';
			info("get message form `%s:%d`: %s", ipaddr_ntoa(addr), port, buffer);
		}
		// send received packet back to sender
		udp_sendto(newpcb, p, addr, port, dest_addr, dest_port);
		// free the pbuf
		pbuf_free(p);
	} while (0);
}

// LWIPProxy的工厂方法，接收新的连接，创建新的lwip的句柄
// 当client与lwip stack完成三次握手之后回调函数
err_t LWIPStack::lwip_proxy_tcp_accept_cb(void* arg, struct tcp_pcb* newpcb, err_t err) {
	info("create `LWIPProxy` instance...");

	if(err != ERR_OK || (newpcb == NULL)) {
		error("LWIPStack::tcp_accept_new_client_cb() failed, errno: %d", err);
		return ERR_VAL; //Illegal value;
	}

	LWIP_UNUSED_ARG(arg);

	// Unless this pcb should have NORMAL priority, set its priority now. When running out of pcbs, 
	// low priority pcbs can be aborted to create new pcbs of higher priority. 
	tcp_setprio(newpcb, TCP_PRIO_MIN);

	info("LWIPStack::tcp_accept_new_client_cb(): new connection `%s:%d`-->`%s:%d`, &tcp_pcb: %p", 
		LOCAL_IP(newpcb), LOCAL_PORT(newpcb), REMOTE_IP(newpcb), REMOTE_PORT(newpcb), newpcb);

	err_t ret;
	
	// 创建代理对象
	// 一个代理对象表示一个pcb实例newpcb和一个远程服务器连接rs
	LWIPProxy* proxy = new LWIPProxy();
	proxy->raw_api_status = raw_api_status_e::ACCEPTED;
	proxy->newpcb = newpcb;
	proxy->retries = 0;
	proxy->pbuf_ptr = nullptr;

	// 写入会话信息
	// info("add session: `%u:%d`-->`%u:%d`, &proxy: %p", LOCAL_IP_U32(newpcb), LOCAL_PORT(newpcb), LOCAL_IP_U32(newpcb), REMOTE_PORT(newpcb), proxy);
	SESSION.insert(LOCAL_IP_U32(newpcb), LOCAL_PORT(newpcb), REMOTE_IP_U32(newpcb), REMOTE_PORT(newpcb), proxy);

	// 与远程服务器建立连接
	proxy->connect_remote_server(newpcb, NatTable::get_instance());

	// 0. 将`proxy`变量作为回调函数的参数`arg`
	tcp_arg(newpcb, proxy);

	// 1. 设置接收回调函数
	tcp_recv(newpcb, [](void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err)->err_t {
		// 未知错误，退出
		if(err != ERR_OK) { 
			error("cleanup, for unknown reason");
			LWIP_ASSERT("no pbuf expected here", p == NULL);
			return err;
		}
		LWIPProxy* proxy = (LWIPProxy*)arg;
		return proxy->tcp_recv_cb(p);
	});
	
	// 2. 设置错误处理回调，因为pcb结构可能已经被删除了，所以在处理错误的回调函数中pcb参数不可能传递进来
	tcp_err(newpcb, [](void *arg, err_t err){
		LWIPProxy* proxy = (LWIPProxy*)arg;
		return proxy->tcp_error_cb(err);
	});
	
	// 3. 设置LWIP定时器
	tcp_poll(newpcb, [](void *arg, struct tcp_pcb *tpcb) {
		LWIPProxy* proxy = (LWIPProxy*)arg;
		return proxy->tcp_poll_cb();
	}, 0);
	
	// 4. 设置发送完成回调
	tcp_sent(newpcb, [](void* arg, struct tcp_pcb* tpcb, u16_t len) {
		LWIPProxy* proxy = (LWIPProxy*)arg;
		return proxy->tcp_sent_cb(len);	
	});
	return ERR_OK;
}

void LWIPStack::lwip_proxy_udp_recv_cb(void *arg, struct udp_pcb *newpcb, struct pbuf *p, const ip_addr_t *addr, u16_t port, const ip_addr_t *dest_addr, u16_t dest_port) {
	do {
		if (newpcb == NULL) {
			break;
		}
		info("send udp packet from: `%s:%d`-->`%s:%d`", ipaddr_ntoa(addr), port, ipaddr_ntoa(dest_addr), dest_port);
		/*
		if (!udp_conn_map_isinited()) {
			set_udp_conn_map(createHashMap(NULL, _equal_udp));
		}

		struct sockaddr_in dst_addr{};
		dst_addr = get_socket_address(ipaddr_ntoa(dest_addr), (u16_t) dest_port);

		char *map_key = get_address_port(dest_addr, port, dest_port);
		udp_conn *conn = (udp_conn *)(udp_conn_map_get(map_key));
		mem_free(map_key);
		if(conn == NULL) {
			if (get_udp_handler() == NULL) {
				LOG("must register a UDP connection handler\n");
				break;
			}
			conn = new_udp_conn(pcb, get_udp_handler(), *addr, port, *dest_addr, dest_port,
								dst_addr);
			if(conn == NULL) {
				break;
			}
		}
		if (p->tot_len == p->len) {
			conn->receive_to(p->payload, p->tot_len, dst_addr);
		} else {
			void *buf = mem_malloc(p->tot_len);
			pbuf_copy_partial(p, buf, p->tot_len, 0);
			conn->receive_to(buf, p->tot_len, dst_addr);
			mem_free(buf);
		}
	*/
	} while (0);
	pbuf_free(p);
}

// Write writes IP packets to the stack.
int LWIPStack::input(u8_t* pkt, int len) {
	debug(">>> LWIPStack::input()...");
	
	if (len == 0) {
		return 0;
	}

	ipver ipv = __peek_ipver(pkt);
	if (ipv <= 0) {
		return 0;
	}

	// Get the protocol type of packetage, like TCP, UDP, ICMP...
	proto next_proto = __peek_nextproto(ipv, pkt, len);
	if(next_proto <= 0) {
		return 0;
	}

	struct pbuf *buf;
	if ((next_proto == proto_udp && !(__more_frags(ipv, pkt))) || __frag_offset(ipv, pkt) > 0) {
		buf = pbuf_alloc_reference(pkt, len, PBUF_REF);
	} else {
		buf = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
		pbuf_take(buf, pkt, len);
	}

	err_t err = (*netif_list).input(buf, netif_list);
	int ret_len = len;
	if (err != ERR_OK) {
		error("packet not handled, errno: %d", err);
		if(buf != NULL) {
			pbuf_free(buf);
		}
		ret_len = 0;
	} else {
		info("packet handled ok");
	}
	debug("<<< LWIPStack::input() done!");
	return ret_len;
}

// 处理IP包的私有辅助函数
ipver LWIPStack::__peek_ipver(u8_t* pkt) {
	return (ipver)((pkt[0] & 0xf0) >> 4);
}

proto LWIPStack::__peek_nextproto(ipver ipv, u8_t *pkt, int len) {
	switch (ipv) {
		case ipv4:
			if(len < 9) {
				info("short IPv4 packet\n");
				break;
			}
			return (proto)pkt[9];
		case ipv6:
			if(len < 6) {
				info("short IPv6 packet\n");
				break;
			}
			return (proto)pkt[6];
		default:
			if(len < 6) {
				info("unknown IP version\n");
				break;
			}
	}
	return 0;
}

u8_t LWIPStack::__more_frags(ipver ipv, u8_t *pkt) {
	switch (ipv) {
		case ipv4:
			// pkt[6] is the offset value of Fragments
			if ((pkt[6] & 0x20) > 0) /* has MF (More Fragments) bit set */ {
				return 1;
			}
		case ipv6:
			// FIXME Just too lazy to implement this for IPv6, for now
			// returning true simply indicate do the copy anyway.
			return 1;
	}
	return 0;
}

u16_t LWIPStack::__frag_offset(ipver ipv, u8_t *pkt) {
	switch (ipv) {
		case ipv4:
			return ((u16_t*)(pkt+6))[0] & 0x1fff;
		case ipv6:
			// FIXME Just too lazy to implement this for IPv6, for now
			// returning a value greater than 0 simply indicate do the
			// copy anyway.
			return 1;
	}
	return 0;
}




