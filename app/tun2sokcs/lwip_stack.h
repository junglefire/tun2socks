#ifndef TUN2SOCKS_LWIP_STACK_H__
#define TUN2SOCKS_LWIP_STACK_H__

#include <lwip/timeouts.h>
#include <lwip/init.h>
#include <lwip/tcp.h>
#include <lwip/udp.h>
#include <lwip/sys.h>
#include <uv.h>

#include "logger.h"

typedef int8_t ipver;
typedef int8_t proto;

const ipver ipv4 = 4;
const ipver ipv6 = 6;
const proto proto_icmp = 1;
const proto proto_tcp  = 6;
const proto proto_udp  = 17;

class LWIPStack {
public:
	LWIPStack();
	~LWIPStack();
	int init(const char*);
	int input(u8_t* data, int len);
	LWIPStack(const LWIPStack&) = delete;
	LWIPStack& operator=(const LWIPStack&) = delete;
private:
	ipver __peek_ipver(u8_t*);
	proto __peek_nextproto(ipver, u8_t*, int);
	u8_t __more_frags(ipver, u8_t*);
	u16_t __frag_offset(ipver, u8_t*);
public:
	struct tcp_pcb* tpcb;
	struct udp_pcb* upcb;
protected:
	uv_timer_t* timeout_handler;
private:
	// 以下是LWIP echo两个工厂方法，接收新的连接，创建新的lwip的句柄
	static err_t lwip_echo_tcp_accept_cb(void*, struct tcp_pcb*, err_t);
	static void lwip_echo_udp_recv_cb(void*, struct udp_pcb*, struct pbuf*, const ip_addr_t*, u16_t, const ip_addr_t*, u16_t);
	// 以下是LWIP proxy两个工厂方法，接收新的连接，创建新的lwip的句柄
	static err_t lwip_proxy_tcp_accept_cb(void*, struct tcp_pcb*, err_t);	
	static void lwip_proxy_udp_recv_cb(void*, struct udp_pcb*, struct pbuf*, const ip_addr_t*, u16_t, const ip_addr_t*, u16_t);
};


#endif // TUN2SOCKS_LWIP_STACK_H__