#ifndef TUN2SOCKS_LWIP_CB_H__
#define TUN2SOCKS_LWIP_CB_H__

#include <lwip/timeouts.h>
#include <lwip/init.h>
#include <lwip/tcp.h>
#include <lwip/udp.h>
#include <lwip/sys.h>
#include <arpa/inet.h>
#include <uv.h>

#include "remote_server.h"
#include "tun2socks.h"
#include "logger.h"

// 从pcb获取ip和端口
#define LOCAL_IP(X) (ipaddr_ntoa(&X->remote_ip))
#define LOCAL_PORT(X) ((u16_t)X->remote_port)
#define REMOTE_IP(X) (ipaddr_ntoa(&X->local_ip))
#define REMOTE_PORT(X) ((u16_t)X->local_port)

// lwip(L) raw(R) api status(S)
typedef enum _lwip_raw_status_e
{
	LRS_NONE = 0,
	LRS_ACCEPTED,
	LRS_RECEIVED,
	LRS_CLOSING
} raw_status_e;

// lwip raw api state
typedef struct _raw_state_t
{
	u8_t state;
	u8_t retries;
	struct tcp_pcb *pcb;
	/* pbuf (chain) to recycle */
	struct pbuf *p;
} raw_state_t;

// lwip(L) proxy(P) api status(S)
typedef enum _lwip_proxy_status_e
{
	LPS_IDLE = 0,
	LPS_CONNECTING,
	LPS_CONNECTED,
	LPS_DISCONNECTING,
	LPS_UNREACHABLE,
	LPS_CLOSE
} proxy_status_e;

// lwip proxy state
class RemoteServer;

typedef struct _proxy_state_t
{
	u8_t state;
	u8_t retries;
	struct tcp_pcb* pcb;
	/* pbuf (chain) to recycle */
	struct pbuf* p;
	RemoteServer* remote_server;
} proxy_state_t;

#endif // TUN2SOCKS_LWIP_CB_H__