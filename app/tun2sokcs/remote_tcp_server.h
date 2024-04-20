#ifndef TUN2SOCKS_REMOTE_SERVER_H__
#define TUN2SOCKS_REMOTE_SERVER_H__

#include <lwip/timeouts.h>
#include <lwip/init.h>
#include <lwip/tcp.h>
#include <lwip/udp.h>
#include <lwip/sys.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <uv.h>

#include <string>

#include "nat_table.h"
#include "common.h"
#include "logger.h"

class RemoteTcpServer {
public:
	RemoteTcpServer();
	~RemoteTcpServer() = default;
	int connect(struct tcp_pcb*, NatTable*);
	int disconnect();
	int send(const char*, size_t);
	remote_server_status_e get_status();
	RemoteTcpServer(const RemoteTcpServer&) = delete;
	RemoteTcpServer& operator=(const RemoteTcpServer&) = delete;
private:
	static void __alloc_cb(uv_handle_t*, size_t, uv_buf_t*);
	int __lwip_to_realserver();
private:
	remote_server_status_e status;	// 远程服务器连接状态
	struct tcp_pcb* newpcb;			// lwip句柄
	NatTable* nat_table;			// NAT表
	std::string cache_buffer;		// 当远程服务器不可用时，缓存消息随后发送
	const char* msg_ptr;			// 待发送的消息指针
	size_t msg_size;				// 待发送的消息大小
private:
	std::string server_ip;			// 服务器IP
	std::string real_ip;			// NAT转换之后的真实IP
	int server_port;				// 服务器端口
private:
	uv_loop_t* loop;
	uv_tcp_t uv_client;
	uv_connect_t uv_connect;
	uv_shutdown_t uv_shutdown_req;
	uv_write_t uv_write_req;
};

#endif // TUN2SOCKS_REMOTE_SERVER_H__