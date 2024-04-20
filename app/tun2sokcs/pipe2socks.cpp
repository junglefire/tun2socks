#include "pipe2socks.h"

uv_stream_t* Pipe2Socks::__client_ptr = nullptr;
uv_write_t* Pipe2Socks::__write_ptr = nullptr;

Pipe2Socks::Pipe2Socks(std::string pipe_name, uv_loop_t* loop, LWIPStack& l): lwip(l) {
	this->pipe_name = pipe_name;
	this->loop = loop;
	this->server.data = static_cast<void*>(this);
	Pipe2Socks::__client_ptr = (uv_stream_t*)(&this->client);
}

Pipe2Socks::~Pipe2Socks() {
	info("stop and close named pipe event loop...");
	// 判断pipe是否已经被删除
	uv_fs_t req;
	// 删除unix域对应的路径
	info("remove named pipe `%s`", PIPENAME);
	uv_fs_unlink(this->loop, &req, PIPENAME, NULL);
}

int Pipe2Socks::start() {
	uv_pipe_init(this->loop, &(this->server), 0);

	int ret = uv_pipe_bind(&(this->server), this->pipe_name.c_str());
	// 绑定unix路径到socket
	if (ret != 0) {
		error("bind named pipe failed, err: %s", uv_err_name(ret));
		return -1;
	}
	// 把unix域对应的文件文件描述符设置为listen状态。
	// 开启监听请求的到来，连接的最大个数是128。有连接时的回调是on_new_connection
	ret = uv_listen((uv_stream_t*)&(this->server), PIPE_MAX_CONN, [](uv_stream_t* server, int status){
		auto p2s = static_cast<Pipe2Socks*>(server->data);
		p2s->pipe_client_connect_cb();
	});

	if (ret != 0) {
		error("named pipe listen failed, err: %s", uv_err_name(ret));
		return -1;
	}

	this->write_pipe();	
	return 0;
}

void Pipe2Socks::pipe_client_connect_cb() {
	info("new pipe peer connected...");

	uv_pipe_init(this->loop, &(this->client), 0);
	this->client.data = this;
	if (uv_accept((uv_stream_t*)&(this->server), (uv_stream_t*)&(this->client)) == 0) {
		// 注册读事件，等待客户端发送信息过来,
		// alloc_buffer分配内存保存客户端的发送过来的信息
		uv_read_start((uv_stream_t*)&(this->client), Pipe2Socks::alloc_buffer, [](uv_stream_t* client, ssize_t nread, const uv_buf_t* buf){
			auto p2s = static_cast<Pipe2Socks*>(client->data);
			p2s->pipe_read_and_write_lwip_cb(nread, buf);
		});
	} else {
		uv_close((uv_handle_t*)&(this->client), NULL);
	}
}

void Pipe2Socks::pipe_read_and_write_lwip_cb(ssize_t nread, const uv_buf_t *buf) {
	uv_stream_t* client = (uv_stream_t*)&(this->client); 
	// 有数据，则回写
	if (nread > 0) {
#ifdef __HEX_DUMP__
		hex_print((unsigned char*)buf->base, nread, "*");
#endif
		// 将IP包写到LWIP协议栈
		Pipe2Socks::__write_ptr = static_cast<uv_write_t*>(malloc(sizeof(uv_write_t)));
		this->lwip.input((u8_t*)buf->base, nread);
		return;
	} else /* (nread <= 0) */ { // 没有数据了，关闭
		info("close named pipe `{}`...", PIPENAME);
		if (nread != UV_EOF) {
			error("read failed, err: %s", uv_err_name(nread));
		}
		// 销毁和客户端通信的结构体，即关闭通信
		uv_close((uv_handle_t*)client, NULL);
	} 
	free(buf->base);
}

void Pipe2Socks::write_pipe() {
	if (netif_list != NULL) {
		netif_list->output = [](struct netif* netif, struct pbuf* p, const ip4_addr_t* ipaddr) {
			return Pipe2Socks::output_tun(p);
		};
		netif_list->output_ip6 = [](struct netif* netif, struct pbuf* p, const ip6_addr_t* ipaddr) {
			return Pipe2Socks::output_tun(p);
		};
	}
}

err_t Pipe2Socks::output_tun(struct pbuf *p) {
	void *buf;
	int ret;
	if(p->tot_len == p->len) {
		buf = (u8_t *)p->payload;
		ret = Pipe2Socks::__output_tun(buf, p->tot_len);
	} else {
		buf = malloc(p->tot_len);
		pbuf_copy_partial(p, buf, p->tot_len, 0);
		ret = Pipe2Socks::__output_tun(buf, p->tot_len);
		free(buf);
	}
	return ret > 0 ? ERR_OK : -1;
}

// 数据回写tun网卡
err_t Pipe2Socks::__output_tun(void *buf, u16_t len) {
	uint32_t type = htonl(AF_INET);

	uv_buf_t s[] = {
    	{ .base = (char*)&type, .len = sizeof(type) },
    	{ .base = (char*)buf, .len = len }
	};

#ifdef __HEX_DUMP__
	hex_print((unsigned char*)s[0].base, s[0].len, "#");
	hex_print((unsigned char*)s[1].base, s[1].len, "#");
#endif 

	int ret = uv_write(Pipe2Socks::__write_ptr, Pipe2Socks::__client_ptr, s, 2, [](uv_write_t *req, int status){
		if (status < 0) {
			error("write failed, err: %s", uv_err_name(status));
		}
		free(Pipe2Socks::__write_ptr);
	});
	return ret == 0 ? 1 : 0;
}

void Pipe2Socks::free_write_req(uv_write_t* req) {
	write_req_t* wr = (write_req_t*)req;
	free(wr->buf.base);
	free(wr);
}

void Pipe2Socks::alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
	*buf = uv_buf_init((char*)malloc(4097), 4097);
}

// [deprecated]
// 测试用的函数
void Pipe2Socks::pipe_read_and_echo_cb(ssize_t nread, const uv_buf_t *buf) {
	uv_stream_t* client = (uv_stream_t*)&(this->client); 
	// 有数据，则回写
	if (nread > 0) {
#ifdef __HEX_DUMP__
		hex_print((unsigned char*)buf->base, nread, "*");
#endif
		write_req_t *req = (write_req_t*) malloc(sizeof(write_req_t));
		// 指向客户端发送过来的数据
		req->buf = uv_buf_init(buf->base, nread);
		// 回写给客户端，echo_write是写成功后的回调
		uv_write((uv_write_t*)req, client, &req->buf, 1, [](uv_write_t *req, int status){
			info("write to named pipe `%s`...", PIPENAME);
			if (status < 0) {
				error("write failed, err: %s", uv_err_name(status));
			}
			Pipe2Socks::free_write_req(req);
		});
		return;
	}
	// 没有数据了，关闭
	if (nread < 0) {
		if (nread != UV_EOF) {
			error("read failed, err: %s", uv_err_name(nread));
		}
		// 销毁和客户端通信的结构体，即关闭通信
		uv_close((uv_handle_t*)client, NULL);
	}
	free(buf->base);
}


