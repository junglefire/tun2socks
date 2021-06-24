#include "remote_server.h"
#include "lwip_proxy_callback.h"

RemoteServer::RemoteServer(uv_loop_t* loop, const char* ip, int port, NatTable* nat_table=nullptr) {
	this->loop = loop;
	this->nat_table = nat_table;
	this->server_ip = ip;
	this->server_port = port;
	this->uv_client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
	this->uv_connect = (uv_connect_t*)malloc(sizeof(uv_connect_t));
	this->uv_connect->data = static_cast<void*>(this);
	this->status = LPS_IDLE;
}

RemoteServer::~RemoteServer() {
	free(this->uv_client);
	free(this->uv_connect);
}

// 连接远程服务器
int RemoteServer::connect() {
	assert(this->uv_client != nullptr);
	assert(this->uv_connect != nullptr);
	assert(this->nat_table != nullptr);
	
	// 查询nat表
	if (this->nat_table) {
		this->real_ip = this->nat_table->query(this->server_ip.c_str());
	} 
	if (this->real_ip.empty()) {
		this->real_ip = this->server_ip;
	}

	info("connecting `%s:%d` ...", this->real_ip.c_str(), this->server_port);

	int ret = 0;
	struct sockaddr_in addr;
	uv_tcp_init(this->loop, this->uv_client);
	// uv_tcp_keepalive(uv_client, 1, 60);
	ret = uv_ip4_addr(this->real_ip.c_str(), this->server_port, &addr);
	if (ret != 0) {
		error("failed to init server address, err: %s", uv_strerror(ret));
		return -1;
	}

	this->status = LPS_CONNECTING;
	ret = uv_tcp_connect(uv_connect, uv_client, (const struct sockaddr*)&addr, [](uv_connect_t* conn, int status) {
		auto rs = static_cast<RemoteServer*>(conn->data);
		if (status < 0) {
			error("failed to dial to server, err: %s", uv_strerror(status));
			rs->status = LPS_CLOSE;
			return; 
		}
		rs->status = LPS_CONNECTED;
		info("server is connectd");
	});

	if (ret != 0) {
		this->status = LPS_UNREACHABLE;
		error("failed to connect to remote server `%s:%d`, err: %s", 
			this->real_ip.c_str(), this->server_port, uv_strerror(ret));
		return -1;
	}
	return 0;
}

int RemoteServer::get_status() {
	return this->status;
}







