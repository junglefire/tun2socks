#include "lwip_echo.h"

// LWIP错误码参考：https://github.com/lwip-tcpip/lwip/blob/master/src/include/lwip/err.h

// 接收client的信息后回调
err_t LWIPEcho::tcp_recv_cb(struct pbuf* new_pbuf) {
	info("LWIPEcho::tcp_recv_cb():`%s:%d`->`%s:%d`", 
		LOCAL_IP(PCB),LOCAL_PORT(PCB),REMOTE_IP(PCB),REMOTE_PORT(PCB));
	
	err_t ret;
	// 如果pbuf为空，说明客户端已经断开连接，参考lwip/tpc.c文件中关于`tcp_recv()`的注释
	if (new_pbuf == NULL) {
		error("client disconnct...");
		this->raw_api_status = raw_api_status_e::CLOSING;
		if(this->pbuf_ptr == NULL) {
			// we're done sending, close it
			this->__tcp_raw_api_close();
		} else {
			// we're not done yet
			LWIPEcho::__tcp_raw_send();
		}
		return ERR_OK;
	} 

	LWIPEcho::__dump_message(new_pbuf);

	if(this->raw_api_status == raw_api_status_e::ACCEPTED) {
		// first data chunk in p->payload
		this->raw_api_status = raw_api_status_e::RECEIVED;
		// store reference to incoming pbuf (chain)
		this->pbuf_ptr = new_pbuf;
		ret = LWIPEcho::__tcp_raw_send();
	} else if (this->raw_api_status == raw_api_status_e::RECEIVED) {
		// read some more data
		if(this->pbuf_ptr == NULL) {
			this->pbuf_ptr = new_pbuf;
			ret = LWIPEcho::__tcp_raw_send();
		} else {
			// chain pbufs to the end of what we recv'ed previously
			struct pbuf* ptr = this->pbuf_ptr;
			pbuf_cat(ptr, new_pbuf);
			ret = ERR_OK;
		}
	} else {
		// unknown ps->state, trash data
		tcp_recved(PCB, new_pbuf->tot_len);
		pbuf_free(new_pbuf);
		ret = ERR_OK;
	}
	return ret;
}

err_t LWIPEcho::tcp_sent_cb(u16_t len) {
	info("LWIPEcho::tcp_sent_cb(): `%s:%d`->`%s:%d`", 
		LOCAL_IP(PCB), LOCAL_PORT(PCB), REMOTE_IP(PCB), REMOTE_PORT(PCB));
	LWIP_UNUSED_ARG(len);
	if(this->pbuf_ptr != NULL) {
		// still got pbufs to send
		// tcp_sent(PCB, tcp_sent_cb);
		this->__tcp_raw_send();
	} else {
		// no more pbufs to send
		if(this->raw_api_status == raw_api_status_e::CLOSING) {
			this->__tcp_raw_api_close();
		}
	}
	return ERR_OK;
}

// 出错时回调
void LWIPEcho::tcp_error_cb(err_t err) {
	info("lwip error, abort!");
	LWIP_UNUSED_ARG(err);
	LWIPEcho::__tcp_raw_state_free();
	// [todo] 是否有更好的方法
	delete this;
}

err_t LWIPEcho::tcp_poll_cb() {
	if (this != NULL) {
		if (this->pbuf_ptr != NULL) {
			// there is a remaining pbuf (chain)
			info("[poll] there is a remaining pbuf (chain), send...");
			this->__tcp_raw_send();
		} else {
			// no remaining pbuf (chain)
			if(this->raw_api_status == raw_api_status_e::CLOSING) {
				this->__tcp_raw_api_close();
			}
		}
	}
	return ERR_OK;

LABEL_ABORT:
	this->__tcp_raw_api_close();
	return ERR_ABRT;
}

// 关闭raw api，删除状态信息存储，关闭pcb
void LWIPEcho::__tcp_raw_api_close() {
	info("destroy connection `%s:%d`->`%s:%d`", LOCAL_IP(PCB), LOCAL_PORT(PCB), REMOTE_IP(PCB), REMOTE_PORT(PCB));
	tcp_arg(PCB, NULL);
	tcp_sent(PCB, NULL);
	tcp_recv(PCB, NULL);
	tcp_err(PCB, NULL);
	tcp_poll(PCB, NULL, 0);

	info("release packet buffer...");
	this->__tcp_raw_state_free();
	
	info("release pcb...");
	tcp_close(PCB);
	
	info("release LWIPEcho instance...");
	// [todo] 是否有更好的方法
	delete this;
}

// 释放状态信息
void LWIPEcho::__tcp_raw_state_free() {
	if (this->pbuf_ptr!=nullptr) {
		// free the buffer chain if present
		pbuf_free(this->pbuf_ptr);
	}
}

// 发送数据1
int LWIPEcho::__tcp_raw_send() {
	struct pbuf* ptr;
	err_t wr_err = ERR_OK;
	while ((wr_err == ERR_OK) && (this->pbuf_ptr != NULL) && (this->pbuf_ptr->len <= tcp_sndbuf(PCB))) {
		ptr = this->pbuf_ptr;
		// enqueue data for transmission
		wr_err = tcp_write(this->newpcb, ptr->payload, ptr->len, 1);
		if (wr_err == ERR_OK) {
			u16_t plen;
			plen = ptr->len;
			// continue with next pbuf in chain (if any)
			this->pbuf_ptr = ptr->next;
			if(this->pbuf_ptr != NULL) {
				// new reference!
				pbuf_ref(this->pbuf_ptr);
			}
			// chop first pbuf from chain
			pbuf_free(ptr);
			// we can read more data now
			tcp_recved(this->newpcb, plen);
		} else if(wr_err == ERR_MEM) {
			// we are low on memory, try later / harder, defer to poll
			this->pbuf_ptr = ptr;
		} else {
			/* other problem ?? */
		}
	}
	return wr_err;
}

// 显示消息内容
void LWIPEcho::__dump_message(struct pbuf* p) {
#ifdef __SHOW_LWIP_RECV_MSG__
	char buffer[1024];
	memset(buffer, 0, sizeof(buffer));
	strncpy(buffer, (char*)p->payload, p->len);
	info("recv `%d` bytes: %s", p->len, buffer);
#endif
}

