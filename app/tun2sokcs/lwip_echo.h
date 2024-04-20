#ifndef TUN2SOCKS_LWIP_ECHO_CLZ_H__
#define TUN2SOCKS_LWIP_ECHO_CLZ_H__

#include <lwip/timeouts.h>
#include <lwip/init.h>
#include <lwip/tcp.h>
#include <lwip/udp.h>
#include <lwip/sys.h>
#include <arpa/inet.h>
#include <uv.h>

#include "tun2socks.h"
#include "common.h"
#include "logger.h"

class LWIPEcho {
public:
	LWIPEcho(const LWIPEcho&) = delete;
	LWIPEcho& operator=(const LWIPEcho&) = delete;
	LWIPEcho() {};
	~LWIPEcho() {};
public:
	err_t tcp_recv_cb(struct pbuf*);
	err_t tcp_sent_cb(u16_t);
	void tcp_error_cb(err_t);
	err_t tcp_poll_cb();
private:
	void __tcp_raw_api_close();
	void __tcp_raw_state_free();
	int __tcp_raw_send();
	static void __dump_message(struct pbuf*);
	// static void __tcp_raw_state_free(echo_raw_api_state_t*);
	// static void __tcp_raw_send(struct tcp_pcb*, echo_raw_api_state_t*);
public:
	raw_api_status_e raw_api_status;
	u8_t retries;
	struct tcp_pcb* newpcb;
	/* pbuf (chain) to recycle */
	struct pbuf* pbuf_ptr;
};


#endif // TUN2SOCKS_LWIP_ECHO_CLZ_H__