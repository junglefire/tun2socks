#include "tun2socks.h"

int Tun2Socks::tun_output_fd = -1;

Tun2Socks::Tun2Socks(int tun_fd, uv_loop_t* loop, LWIPStack& l): lwip(l) {
	this->tun_fd = tun_fd;
	this->loop = loop;
	this->poll = (uv_poll_t*)malloc(sizeof(uv_poll_t));
	this->status = STOP;
	Tun2Socks::tun_output_fd = tun_fd;
}

Tun2Socks::~Tun2Socks() {
	info("stop and close tun event loop...");
	if (this->poll) {
		if (this->status == RUNNING) {
			uv_poll_stop(this->poll);
			uv_close((uv_handle_t*)this->poll, NULL);	
		}
		free(this->poll);
	}
}

// 启动tun事件处理器，使用uv_poll_t
int Tun2Socks::start() {
	info("tun event loop run...");
	if (this->tun_fd <= 0) {
		error("tun_fd is invalid, tun_fd= %d", tun_fd);
		return -1;
	}

	int ret = uv_poll_init_socket(this->loop, this->poll, this->tun_fd);
	if (ret != 0) {
		error("uv_poll_init_socket() failed, err: %s", uv_strerror(errno));
		return -1;
	}

	this->poll->data = this;
	// `UV_WRITABLE|UV_PRIORITIZED|UV_DISCONNECT`会导致cpu占用100%
	// ret = uv_poll_start(this->poll, UV_READABLE/*|UV_WRITABLE|UV_PRIORITIZED|UV_DISCONNECT*/, &Tun2Socks::on_poll_cb);
	ret = uv_poll_start(this->poll, UV_READABLE, [](uv_poll_s* handle, int status, int events) {
		Tun2Socks* t2s = (Tun2Socks*)(handle->data);
		t2s->read_device(handle, status, events);
	});

	if (ret != 0) {
		error("uv_poll_start() failed, err: %s", uv_strerror(errno));
		return -1;
	}

	this->write_device();
	this->status = RUNNING;
	return 0;
}

// poll事件回调
void Tun2Socks::read_device(uv_poll_s* handle, int status, int events) {
	int tun_fd = this->tun_fd;

	// LWIPStack::input()将`buffer`内容拷贝到lwip的内存空间，这里使用局部变量没有问题
	char buffer[MTU];
	Tun2Socks* t2s = static_cast<Tun2Socks*>(handle->data);

	debug("read tun device...");

	// 读取tun的数据
	if (!status && (events & UV_READABLE)) {
		ssize_t length = read(tun_fd, buffer, MTU);

#ifdef __HEX_DUMP__
		hex_print((unsigned char*)buffer, length, "^");
#endif
		if(length < 0) {
			error("tun_read:(len < 0), err: %s", strerror(errno));
		} else if (length == 0) {
			error("tun is empty");
		} else if(length > 0) {
			info("get a message from tun device...");
			t2s->lwip.input((u8_t*)buffer+4, length);
		}
	}
}

void Tun2Socks::write_device() {
	if (netif_list != NULL) {
		netif_list->output = [](struct netif *netif, struct pbuf *p, const ip4_addr_t *ipaddr) {
			return Tun2Socks::output_tun(p);
		};
		netif_list->output_ip6 = [](struct netif *netif, struct pbuf *p, const ip6_addr_t *ipaddr) {
			return Tun2Socks::output_tun(p);
		};
	}
}

err_t Tun2Socks::output_tun(struct pbuf *p) {
	void *buf;
	int ret;
	if(p->tot_len == p->len) {
		buf = (u8_t *)p->payload;
		ret = Tun2Socks::__output_tun(buf, p->tot_len);
	} else {
		buf = malloc(p->tot_len);
		pbuf_copy_partial(p, buf, p->tot_len, 0);
		ret = Tun2Socks::__output_tun(buf, p->tot_len);
		free(buf);
	}
	return ret > 0 ? ERR_OK : -1;
}

// 数据回写tun网卡
err_t Tun2Socks::__output_tun(void *buf, u16_t len) {
	struct iovec iv[2];
	uint32_t type = htonl(AF_INET);
	
	iv[0].iov_base = (uint8_t *)&type;
	iv[0].iov_len = sizeof(type);
	iv[1].iov_base = (uint8_t *)buf;
	iv[1].iov_len = len;

#ifdef __HEX_DUMP__
	hex_print((unsigned char*)iv[0].iov_base, iv[0].iov_len, "#");
	hex_print((unsigned char*)iv[1].iov_base, iv[1].iov_len, "#");
#endif

	int wlen = writev(Tun2Socks::tun_output_fd, iv, ARRAY_SIZE(iv));
	info("write %d bytes to tun device", wlen);
	return wlen > 0 ? 1 : 0;
}



