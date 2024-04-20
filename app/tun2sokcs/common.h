#ifndef TUN2SOCKS_STATUS_H__
#define TUN2SOCKS_STATUS_H__

#include <string>
#include <vector>

#define MTU_USER_DEFINE 1500
#define PIPENAME 		"/tmp/pipe2socks.pipe"
#define PIPE_MAX_CONN 	128
#define PCB 			(this->newpcb)

// 从pcb获取ip和端口
#define LOCAL_IP(X) (ipaddr_ntoa(&X->remote_ip))
#define LOCAL_IP_U32(X) (((X)->remote_ip.u_addr.ip4.addr))
#define LOCAL_PORT(X) ((u16_t)X->remote_port)

#define REMOTE_IP(X) (ipaddr_ntoa(&X->local_ip))
#define REMOTE_IP_U32(X) (((X)->local_ip.u_addr.ip4.addr))
#define REMOTE_PORT(X) ((u16_t)X->local_port)

// lwip(L) raw(R) api status(S)
enum class raw_api_status_e {
	NONE = 0,
	ACCEPTED,
	RECEIVED,
	CLOSING
};

// remote server status(S)
enum class remote_server_status_e {
	IDLE = 0,
	CONNECTING,
	CONNECTED,
	DISCONNECTING,
	UNREACHABLE,
	CLOSE
};

#endif // TUN2SOCKS_STATUS_H__