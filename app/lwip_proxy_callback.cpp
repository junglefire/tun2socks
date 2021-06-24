#include "lwip_proxy_callback.h"
#include "remote_server.h"

// LWIP错误码参考：https://github.com/lwip-tcpip/lwip/blob/master/src/include/lwip/err.h

// 当client与lwip stack完成三次握手之后回调函数
err_t LWIPProxyCallback::tcp_accept_cb(void* arg, struct tcp_pcb* newpcb, err_t err) {
	if(err != ERR_OK || (newpcb == NULL)) {
		error("tcp_accept_cb() failed, errno: %d", err);
		return ERR_VAL; //Illegal value;
	}

	LWIP_UNUSED_ARG(arg);

	// Unless this pcb should have NORMAL priority, set its priority now.
	// When running out of pcbs, low priority pcbs can be aborted to create
	// new pcbs of higher priority. 
	tcp_setprio(newpcb, TCP_PRIO_MIN);

	info("LWIPProxyCallback::tcp_accept_cb(): new connection `%s:%d`-->`%s:%d`, &tcp_pcb: %p", 
		LOCAL_IP(newpcb), LOCAL_PORT(newpcb), REMOTE_IP(newpcb), REMOTE_PORT(newpcb), newpcb);

	err_t ret_err;
	proxy_state_t* ps = (proxy_state_t*)mem_malloc(sizeof(proxy_state_t));
	if (ps == NULL) {
		error("allocate raw api state failed, abort!");
		return ERR_MEM;
	}

	ps->state = LRS_ACCEPTED;
	ps->pcb = newpcb;
	ps->retries = 0;
	ps->p = nullptr;
	ps->remote_server = nullptr;

	// 与远程服务器建立连接
	RemoteServer* rs = new RemoteServer(uv_default_loop(), REMOTE_IP(newpcb), REMOTE_PORT(newpcb), NatTable::get_instance());
	rs->connect();
	ps->remote_server = rs;

	// pass newly allocated `ps` to our callbacks
	tcp_arg(newpcb, ps);
	tcp_recv(newpcb, LWIPProxyCallback::tcp_recv_cb);
	tcp_err(newpcb, LWIPProxyCallback::tcp_error_cb);
	tcp_poll(newpcb, LWIPProxyCallback::tcp_poll_cb, 0);
	tcp_sent(newpcb, LWIPProxyCallback::tcp_sent_cb);
	return ERR_OK;
}

// 接收client的信息后回调
err_t LWIPProxyCallback::tcp_recv_cb(void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err) {
	info("LWIPProxyCallback::tcp_recv_cb(): `%s:%d`->`%s:%d`, err: %d, &tcp_pcb: %p, &pbuf: %p", 
		LOCAL_IP(tpcb), LOCAL_PORT(tpcb), REMOTE_IP(tpcb), REMOTE_PORT(tpcb), err, tpcb, p);
	LWIP_ASSERT("arg != NULL", arg != NULL);

	err_t ret;
	proxy_state_t* ps = (proxy_state_t*)arg;

	// 如果pbuf为空，说明客户端已经断开连接，参考lwip/tpc.c文件中关于`tcp_recv()`的注释
	if (p == NULL) {
		error("client disconnct...");
		ps->state = LRS_CLOSING;
		if(ps->p == NULL) {
			// we're done sending, close it
			LWIPProxyCallback::__tcp_raw_close(tpcb, ps);
		} else {
			// we're not done yet
			LWIPProxyCallback::__tcp_raw_send(tpcb, ps);
		}
		return ERR_OK;
	} 

	// 未知错误，退出
	if(err != ERR_OK) { 
		error("cleanup, for unknown reason");
		LWIP_ASSERT("no pbuf expected here", p == NULL);
		return err;
	}

#ifdef __SHOW_LWIP_RECV_MSG__
	char buffer[1024];
	memset(buffer, 0, sizeof(buffer));
	strncpy(buffer, (char*)p->payload, p->len);
	info("recv `%d` bytes: %s", p->len, buffer);
#endif

	if(ps->state == LRS_ACCEPTED) {
		// first data chunk in p->payload
		ps->state = LRS_RECEIVED;
		// store reference to incoming pbuf (chain)
		ps->p = p;
		ret = LWIPProxyCallback::__tcp_raw_send(tpcb, ps);
	} else if (ps->state == LRS_RECEIVED) {
		// read some more data
		if(ps->p == NULL) {
			ps->p = p;
			ret = LWIPProxyCallback::__tcp_raw_send(tpcb, ps);
		} else {
			struct pbuf* ptr;
			// chain pbufs to the end of what we recv'ed previously
			ptr = ps->p;
			pbuf_cat(ptr, p);
			ret = ERR_OK;
		}
	} else {
		// unknown ps->state, trash data
		tcp_recved(tpcb, p->tot_len);
		pbuf_free(p);
		ret = ERR_OK;
	}
	return ret;
}

// 向client发送完数据后回调
err_t LWIPProxyCallback::tcp_sent_cb(void* arg, struct tcp_pcb* tpcb, u16_t len) {
	info("LWIPProxyCallback::tcp_sent_cb(): `%s:%d`->`%s:%d`", 
		LOCAL_IP(tpcb), LOCAL_PORT(tpcb), REMOTE_IP(tpcb), REMOTE_PORT(tpcb));

	LWIP_UNUSED_ARG(len);

	proxy_state_t* rs = (proxy_state_t*)arg;
	rs->retries = 0;
	if(rs->p != NULL) {
		// still got pbufs to send
		tcp_sent(tpcb, tcp_sent_cb);
		LWIPProxyCallback::__tcp_raw_send(tpcb, rs);
	} else {
		// no more pbufs to send
		if(rs->state == LRS_CLOSING) {
			LWIPProxyCallback::__tcp_raw_close(tpcb, rs);
		}
	}
	return ERR_OK;
}

// 出错时回调
void LWIPProxyCallback::tcp_error_cb(void *arg, err_t err) {
	LWIP_UNUSED_ARG(err);
	proxy_state_t* ps = (proxy_state_t*)arg;
	LWIPProxyCallback::__tcp_raw_state_free(ps);
}

// 定时任务
err_t LWIPProxyCallback::tcp_poll_cb(void *arg, struct tcp_pcb *tpcb) {
	err_t ret_err = ERR_OK;
	proxy_state_t* ps = (proxy_state_t*)arg;

	// 检查远点服务器的连接状态
	if (ps->remote_server->get_status() != LPS_CONNECTED) {
		info("[poll] remote server connecting...");
		return ERR_CONN;
	}

	if (ps != NULL) {
		if (ps->p != NULL) {
			// there is a remaining pbuf (chain)
			info("[poll] there is a remaining pbuf (chain), send...");
			LWIPProxyCallback::__tcp_raw_send(tpcb, ps);
		} else {
			// no remaining pbuf (chain)
			if(ps->state == LRS_CLOSING) {
				LWIPProxyCallback::__tcp_raw_close(tpcb, ps);
			}
		}
		ret_err = ERR_OK;
	} else {
		// nothing to be done
		tcp_abort(tpcb);
		ret_err = ERR_ABRT;
	}
	return ret_err;
}

// 关闭raw api，删除状态信息存储，关闭pcb
void LWIPProxyCallback::__tcp_raw_close(struct tcp_pcb* tpcb, proxy_state_t* rs) {
	info("destroy connection `%s:%d`->`%s:%d`, &tcp_pcb: %p", 
		LOCAL_IP(tpcb), LOCAL_PORT(tpcb), REMOTE_IP(tpcb), REMOTE_PORT(tpcb), tpcb);
	tcp_arg(tpcb, NULL);
	tcp_sent(tpcb, NULL);
	tcp_recv(tpcb, NULL);
	tcp_err(tpcb, NULL);
	tcp_poll(tpcb, NULL, 0);
	LWIPProxyCallback::__tcp_raw_state_free(rs);
	tcp_close(tpcb);
}

// 释放状态信息
void LWIPProxyCallback::__tcp_raw_state_free(proxy_state_t* ps) {
	if (ps != NULL) {
		if (ps->p) {
			// free the buffer chain if present
			pbuf_free(ps->p);
		}
		mem_free(ps);
	}
}

// 发送数据1
int LWIPProxyCallback::__tcp_raw_send(struct tcp_pcb* tpcb, proxy_state_t* ps) {
	struct pbuf* ptr;
	err_t ret = ERR_OK;

	while ((ret == ERR_OK) && (ps->p != NULL) && (ps->p->len <= tcp_sndbuf(tpcb))) {
		ptr = ps->p;
#ifdef __SHOW_LWIP_RECV_MSG__
		char buffer[1024];
		memset(buffer, 0, sizeof(buffer));
		strncpy(buffer, (char*)ptr->payload, ptr->len);
		info("send `%d` bytes: %s", ptr->len, buffer);
#endif
		// enqueue data for transmission
		ret = tcp_write(tpcb, ptr->payload, ptr->len, 1);
		if (ret == ERR_OK) {
			u16_t plen;
			plen = ptr->len;
			// continue with next pbuf in chain (if any)
			ps->p = ptr->next;
			if(ps->p != NULL) {
				// new reference!
				pbuf_ref(ps->p);
			}
			// chop first pbuf from chain
			pbuf_free(ptr);
			// we can read more data now
			// tcp_recved(tpcb, plen);
		} else if(ret == ERR_MEM) {
			// we are low on memory, try later / harder, defer to poll
			ps->p = ptr;
		} else {
			/* other problem ?? */
		}
	}
	return ret;
}

// 发送数据2
void LWIPProxyCallback::__tcp_raw_send_2(struct tcp_pcb* tpcb, proxy_state_t* ps) {
	struct pbuf* ptr;
	err_t wr_err = ERR_OK;

	while ((wr_err == ERR_OK) && (ps->p != NULL) && (ps->p->len <= tcp_sndbuf(tpcb))) {
		ptr = ps->p;
#ifdef __SHOW_LWIP_RECV_MSG__
		char buffer[1024];
		memset(buffer, 0, sizeof(buffer));
		strncpy(buffer, (char*)ptr->payload, ptr->len);
		info("send `%d` bytes: %s", ptr->len, buffer);
#endif
		// enqueue data for transmission
		wr_err = tcp_write(tpcb, ptr->payload, ptr->len, 1);
		if (wr_err == ERR_OK) {
			u16_t plen;
			plen = ptr->len;
			// continue with next pbuf in chain (if any)
			ps->p = ptr->next;
			if(ps->p != NULL) {
				// new reference!
				pbuf_ref(ps->p);
			}
			// chop first pbuf from chain
			pbuf_free(ptr);
			// we can read more data now
			tcp_recved(tpcb, plen);
		} else if(wr_err == ERR_MEM) {
			// we are low on memory, try later / harder, defer to poll
			ps->p = ptr;
		} else {
			/* other problem ?? */
		}
	}
}

void LWIPProxyCallback::udp_recv_cb(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port, const ip_addr_t *dest_addr, u16_t dest_port) {
	do {
		if (pcb == NULL) {
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


