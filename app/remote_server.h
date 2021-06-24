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
#include "tun2socks.h"
#include "logger.h"

class RemoteServer {
public:
	RemoteServer(uv_loop_t*, const char*, int, NatTable*);
	~RemoteServer();
	int connect();
	int get_status();
private:
	uv_loop_t* loop;
	NatTable* nat_table;
	std::string server_ip;
	std::string real_ip;
	int server_port;
	uv_tcp_t* uv_client;
	uv_connect_t* uv_connect;
	int status;
private:
	RemoteServer(const RemoteServer&);
	RemoteServer& operator=(const RemoteServer&);
};

#endif // TUN2SOCKS_REMOTE_SERVER_H__