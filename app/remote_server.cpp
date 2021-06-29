#include "remote_server.h"

RemoteServer::RemoteServer() {
	this->loop = uv_default_loop();
	this->uv_connect.data = static_cast<void*>(this);
	this->status = remote_server_status_e::IDLE;
}

// 连接远程服务器
int RemoteServer::connect(struct tcp_pcb* newpcb, NatTable* nat_table) {
	assert(newpcb != nullptr);
	this->newpcb = newpcb;
	this->nat_table = nat_table;
	this->server_ip = REMOTE_IP(newpcb);
	this->server_port = REMOTE_PORT(newpcb);
	
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
	uv_tcp_init(this->loop, &this->uv_client);
	// uv_tcp_keepalive(uv_client, 1, 60);
	ret = uv_ip4_addr(this->real_ip.c_str(), this->server_port, &addr);
	if (ret != 0) {
		error("failed to init server address, err: %s", uv_strerror(ret));
		this->status = remote_server_status_e::UNREACHABLE;
		return -1;
	}

	this->status = remote_server_status_e::CONNECTING;
	
	ret = uv_tcp_connect(&uv_connect, &uv_client, (const struct sockaddr*)&addr, [](uv_connect_t* conn, int status) {
		auto rs = static_cast<RemoteServer*>(conn->data);
		if (status < 0) {
			error("failed to dial to server, err: %s", uv_strerror(status));
			rs->status = remote_server_status_e::UNREACHABLE;
			return; 
		}
		rs->status = remote_server_status_e::CONNECTED;
		if (rs->cache_buffer.length() > 0) {
			info("send cached message: %s", rs->cache_buffer.c_str());
			rs->send(rs->cache_buffer.c_str(), rs->cache_buffer.length());
			rs->cache_buffer.clear();
		}
		info("server is connectd");
	});

	if (ret != 0) {
		this->status = remote_server_status_e::UNREACHABLE;
		error("failed to connect to remote server `%s:%d`, err: %s", 
			this->real_ip.c_str(), this->server_port, uv_strerror(ret));
		return -1;
	}
	return 0;
}

// 断开远程服务器
int RemoteServer::disconnect() {
	info("disconnect remote server `%s:%d`...", this->server_ip.c_str(), this->server_port);
	if (this->status != remote_server_status_e::CONNECTED) {
		return 0;
	}

	int ret = uv_shutdown(&this->uv_shutdown_req, uv_connect.handle, [](uv_shutdown_t* req, int status){
		info("uv_shutdown callback...");
	});

	uv_close((uv_handle_t*)(this->uv_connect.handle), [](uv_handle_t* handle){
		info("uv_close callback...");
	});
	return ret;
}

int RemoteServer::send(const char* buffer, size_t size) {
	if (this->status == remote_server_status_e::CONNECTING) {
		info("remote server is connecting, cache `%d` bytes", size);
		this->cache_buffer.append(buffer, size);
		return 0;
	} 

	if (this->status != remote_server_status_e::CONNECTED) {
		info("remote server is in invalid state, abort!");
		return -1;
	}

	this->msg_ptr = buffer;
	this->msg_size = size;
	this->__lwip_to_realserver();
	return 0;
}

remote_server_status_e RemoteServer::get_status() {
	return this->status;
}

int RemoteServer::__lwip_to_realserver() {
	uv_buf_t iov[] = {
		{.base = const_cast<char*>(this->msg_ptr), .len = this->msg_size}
	};
	this->uv_write_req.data = this;

	return uv_write(&this->uv_write_req, this->uv_connect.handle, iov, 1, [](uv_write_t* req, int status) {
		info("uv_write callback, status: %d", status);
		auto rs = static_cast<RemoteServer*>(req->data);
		uv_stream_t* stream = rs->uv_connect.handle;
		stream->data = rs;
		// tcp_write(rs->newpcb, rs->msg_ptr, rs->msg_size, 1);
		// info("uv_read `%d` bytes: %s", rs->msg_size, rs->msg_ptr);
		uv_read_start(stream, RemoteServer::__alloc_cb, [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf){
			auto rs = static_cast<RemoteServer*>(stream->data);
			if(nread >= 0) {
				tcp_write(rs->newpcb, buf->base, nread, 1);
				info("uv_read `%d` bytes: %s", nread, buf->base);
			} else {
				uv_close((uv_handle_t*)stream, [](uv_handle_t* handle) {
					info("remote server closed");
				});
				error("read from backend server failed, remote server disconnecting...");
			}
			//cargo-culted
			free(buf->base);
		});
	});
}

void RemoteServer::__alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
	buf->len = suggested_size;
	buf->base = (char *)malloc(suggested_size);
}









