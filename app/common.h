#ifndef TUN2SOCKS_STATUS_H__
#define TUN2SOCKS_STATUS_H__

// 从pcb获取ip和端口
#define LOCAL_IP(X) (ipaddr_ntoa(&X->remote_ip))
#define LOCAL_PORT(X) ((u16_t)X->remote_port)
#define REMOTE_IP(X) (ipaddr_ntoa(&X->local_ip))
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