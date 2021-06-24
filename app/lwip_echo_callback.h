#ifndef TUN2SOCKS_LWIP_ECHO_CB_H__
#define TUN2SOCKS_LWIP_ECHO_CB_H__

#include <lwip/timeouts.h>
#include <lwip/init.h>
#include <lwip/tcp.h>
#include <lwip/udp.h>
#include <lwip/sys.h>
#include <arpa/inet.h>
#include <uv.h>

#include "lwip_callback.h"
#include "tun2socks.h"
#include "logger.h"

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
	static void __tcp_raw_close(struct tcp_pcb*, raw_state_t*);
	static void __tcp_raw_state_free(raw_state_t*);
	static void __tcp_raw_send(struct tcp_pcb*, raw_state_t*);
private:
	LWIPEchoCallback(const LWIPEchoCallback&);
	LWIPEchoCallback& operator=(const LWIPEchoCallback&);
};


#endif // TUN2SOCKS_LWIP_CB_H__