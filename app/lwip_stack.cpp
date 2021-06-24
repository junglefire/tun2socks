#include "lwip_proxy_callback.h"
#include "lwip_echo_callback.h"
#include "lwip_stack.h"
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

int LWIPStack::init() {
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

	if (NatTable::get_instance()->is_nat_active()) {
		info("register lwip proxy callback...");
		tcp_accept(this->tpcb, LWIPProxyCallback::tcp_accept_cb);
		udp_recv(this->upcb, LWIPProxyCallback::udp_recv_cb, NULL);
	} else {
		info("register lwip echo callback...");
		tcp_accept(this->tpcb, LWIPEchoCallback::tcp_accept_cb);
		udp_recv(this->upcb, LWIPEchoCallback::udp_recv_cb, NULL);
	}
	return 0;
}

// Write writes IP packets to the stack.
int LWIPStack::input(u8_t* pkt, int len) {
	debug("LWIPStack::input()...");
	
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




