#ifndef TUN2SOCKS_LWIP_PROXY_H__
#define TUN2SOCKS_LWIP_PROXY_H__

#include <lwip/timeouts.h>
#include <lwip/init.h>
#include <lwip/tcp.h>
#include <lwip/udp.h>
#include <lwip/sys.h>
#include <arpa/inet.h>
#include <uv.h>

#include "remote_tcp_server.h"
#include "conn_session.h"
#include "nat_table.h"
#include "logger.h"

class LWIPProxy {
public:
	LWIPProxy();
	~LWIPProxy() = default;
	LWIPProxy(const LWIPProxy&) = delete;
	LWIPProxy& operator=(const LWIPProxy&) = delete;
	
	int connect_remote_server(struct tcp_pcb*, NatTable*);
	int disconnect_remote_server();
public:
	err_t tcp_recv_cb(struct pbuf*);
	err_t tcp_sent_cb(u16_t);
	void tcp_error_cb(err_t);
	err_t tcp_poll_cb();
public:
	RemoteTcpServer rs;
	raw_api_status_e raw_api_status;
	int retries;
	struct tcp_pcb* newpcb;
	/* pbuf (chain) to recycle */
	struct pbuf* pbuf_ptr;
	int connect_retries;
private:
	void __tcp_raw_api_close();
	void __tcp_raw_state_free();
	int __tcp_raw_send();
	static void __dump_message(struct pbuf*);
};


#endif // TUN2SOCKS_LWIP_PROXY_CB_H__