#ifndef TUN2SOCKS_LWIP_ECHO_CB_H__
#define TUN2SOCKS_LWIP_ECHO_CB_H__

#include <lwip/timeouts.h>
#include <lwip/init.h>
#include <lwip/tcp.h>
#include <lwip/udp.h>
#include <lwip/udp.h>
#include <lwip/sys.h>
#include <arpa/inet.h>
#include <uv.h>

#include "tun2socks.h"
#include "common.h"
#include "logger.h"

// lwip raw api state
typedef struct _echo_raw_api_state_t
{
	raw_api_status_e state;
	u8_t retries;
	struct tcp_pcb* pcb;
	/* pbuf (chain) to recycle */
	struct pbuf* packet_buffer;
} echo_raw_api_state_t;

class LWIPEchoCallback {
public:
	LWIPEchoCallback() {};
	~LWIPEchoCallback() {};
public:
	static err_t tcp_accept_cb(void*, struct tcp_pcb*, err_t);
	static err_t tcp_recv_cb(void*, struct tcp_pcb*, struct pbuf*, err_t);
	static err_t tcp_sent_cb(void*, struct tcp_pcb*, u16_t);
	static void tcp_error_cb(void*, err_t);
	static err_t tcp_poll_cb(void*, struct tcp_pcb*);
	static void udp_recv_cb(void*, struct udp_pcb*, struct pbuf*, const ip_addr_t*, u16_t, const ip_addr_t*, u16_t);
private:
	static void __tcp_raw_close(struct tcp_pcb*, echo_raw_api_state_t*);
	static void __tcp_raw_state_free(echo_raw_api_state_t*);
	static void __tcp_raw_send(struct tcp_pcb*, echo_raw_api_state_t*);
private:
	LWIPEchoCallback(const LWIPEchoCallback&) = delete;
	LWIPEchoCallback& operator=(const LWIPEchoCallback&) = delete;
};


#endif // TUN2SOCKS_LWIP_CB_H__