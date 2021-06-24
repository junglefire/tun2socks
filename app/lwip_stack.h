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
	int init();
	int input(u8_t* data, int len);
	int finish();
	void restart_timeouts();
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
	LWIPStack(const LWIPStack&);
	LWIPStack& operator=(const LWIPStack&);
};


#endif // TUN2SOCKS_LWIP_STACK_H__