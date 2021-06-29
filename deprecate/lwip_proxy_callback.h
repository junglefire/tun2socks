#ifndef TUN2SOCKS_LWIP_PROXY_CB_H__
#define TUN2SOCKS_LWIP_PROXY_CB_H__

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

class LWIPProxyCallback {
public:
	LWIPProxyCallback() {};
	~LWIPProxyCallback() {};
public:
	static err_t tcp_recv_cb(void*, struct tcp_pcb*, struct pbuf*, err_t);
	static err_t tcp_sent_cb(void*, struct tcp_pcb*, u16_t);
	static void tcp_error_cb(void*, err_t);
	static err_t tcp_poll_cb(void*, struct tcp_pcb*);
private:
	static void __tcp_raw_close(struct tcp_pcb*, proxy_state_t*);
	static void __tcp_raw_state_free(proxy_state_t*);
	static int __tcp_raw_send(struct tcp_pcb*, proxy_state_t*);
	static void __tcp_raw_send_2(struct tcp_pcb*, proxy_state_t*);
private:
	LWIPProxyCallback(const LWIPProxyCallback&);
	LWIPProxyCallback& operator=(const LWIPProxyCallback&);
};


#endif // TUN2SOCKS_LWIP_PROXY_CB_H__