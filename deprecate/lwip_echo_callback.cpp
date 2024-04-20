#include "lwip_echo_callback.h"

// LWIP错误码参考：https://github.com/lwip-tcpip/lwip/blob/master/src/include/lwip/err.h

// 当client与lwip stack完成三次握手之后回调函数
err_t LWIPEchoCallback::tcp_accept_cb(void* arg, struct tcp_pcb* newpcb, err_t err) {
	if(err != ERR_OK || (newpcb == NULL)) {
		error("tcp_accept_cb() failed, errno: %d", err);
		return ERR_VAL; //Illegal value;
	}

	LWIP_UNUSED_ARG(arg);

	// Unless this pcb should have NORMAL priority, set its priority now.
	// When running out of pcbs, low priority pcbs can be aborted to create
	// new pcbs of higher priority. 
	tcp_setprio(newpcb, TCP_PRIO_MIN);

	info("LWIPEchoCallback::tcp_accept_cb(): new connection `%s:%d`-->`%s:%d`, &tcp_pcb: %p", 
		LOCAL_IP(newpcb), LOCAL_PORT(newpcb), REMOTE_IP(newpcb), REMOTE_PORT(newpcb), newpcb);

	err_t ret_err;
	echo_raw_api_state_t* rs = (echo_raw_api_state_t*)mem_malloc(sizeof(echo_raw_api_state_t));
	if (rs == NULL) {
		error("allocate raw api state failed, abort!");
		return ERR_MEM;
	}

	rs->state = raw_api_status_e::ACCEPTED;
	rs->pcb = newpcb;
	rs->retries = 0;
	rs->packet_buffer = NULL;
	// pass newly allocated `rs` to our callbacks
	tcp_arg(newpcb, rs);
	tcp_recv(newpcb, LWIPEchoCallback::tcp_recv_cb);
	tcp_err(newpcb, LWIPEchoCallback::tcp_error_cb);
	tcp_poll(newpcb, LWIPEchoCallback::tcp_poll_cb, 0);
	tcp_sent(newpcb, LWIPEchoCallback::tcp_sent_cb);
	return ERR_OK;
}

// 接收client的信息后回调
err_t LWIPEchoCallback::tcp_recv_cb(void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err) {
	info("LWIPEchoCallback::tcp_recv_cb(): `%s:%d`->`%s:%d`, err: %d, &tcp_pcb: %p", 
		LOCAL_IP(tpcb), LOCAL_PORT(tpcb), REMOTE_IP(tpcb), REMOTE_PORT(tpcb), err, tpcb);
	LWIP_ASSERT("arg != NULL", arg != NULL);
	
	err_t ret_err;
	echo_raw_api_state_t* rs = (echo_raw_api_state_t*)arg;

	// 如果pbuf为空，说明客户端已经断开连接，参考lwip/tpc.c文件中关于`tcp_recv()`的注释
	if (p == NULL) {
		error("client disconnct...");
		rs->state = raw_api_status_e::CLOSING;
		if(rs->packet_buffer == NULL) {
			// we're done sending, close it
			LWIPEchoCallback::__tcp_raw_close(tpcb, rs);
		} else {
			// we're not done yet
			LWIPEchoCallback::__tcp_raw_send(tpcb, rs);
		}
		ret_err = ERR_OK;
	} else if(err != ERR_OK) { 
		error("cleanup, for unknown reason");
		LWIP_ASSERT("no pbuf expected here", p == NULL);
		ret_err = err;
	} else if(rs->state == raw_api_status_e::ACCEPTED) {
		// first data chunk in p->payload
		rs->state = raw_api_status_e::RECEIVED;
		// store reference to incoming pbuf (chain)
		rs->packet_buffer = p;
		LWIPEchoCallback::__tcp_raw_send(tpcb, rs);
		ret_err = ERR_OK;
	} else if (rs->state == raw_api_status_e::RECEIVED) {
		// read some more data
		if(rs->packet_buffer == NULL) {
			rs->packet_buffer = p;
			LWIPEchoCallback::__tcp_raw_send(tpcb, rs);
		} else {
			struct pbuf* ptr;
			// chain pbufs to the end of what we recv'ed previously
			ptr = rs->packet_buffer;
			pbuf_cat(ptr,p);
		}
		ret_err = ERR_OK;
	} else {
		// unknown rs->state, trash data
		tcp_recved(tpcb, p->tot_len);
		pbuf_free(p);
		ret_err = ERR_OK;
	}
	return ret_err;
}

// 向client发送完数据后回调
err_t LWIPEchoCallback::tcp_sent_cb(void* arg, struct tcp_pcb* tpcb, u16_t len) {
	info("LWIPEchoCallback::tcp_sent_cb(): `%s:%d`->`%s:%d`", 
		LOCAL_IP(tpcb), LOCAL_PORT(tpcb), REMOTE_IP(tpcb), REMOTE_PORT(tpcb));

	LWIP_UNUSED_ARG(len);

	echo_raw_api_state_t* rs = (echo_raw_api_state_t*)arg;
	rs->retries = 0;
	if(rs->packet_buffer != NULL) {
		// still got pbufs to send
		tcp_sent(tpcb, tcp_sent_cb);
		LWIPEchoCallback::__tcp_raw_send(tpcb, rs);
	} else {
		// no more pbufs to send
		if(rs->state == raw_api_status_e::CLOSING) {
			LWIPEchoCallback::__tcp_raw_close(tpcb, rs);
		}
	}
	return ERR_OK;
}

// 出错时回调
void LWIPEchoCallback::tcp_error_cb(void *arg, err_t err) {
	LWIP_UNUSED_ARG(err);
	echo_raw_api_state_t* rs = (echo_raw_api_state_t*)arg;
	LWIPEchoCallback::__tcp_raw_state_free(rs);
}

// 定时任务
err_t LWIPEchoCallback::tcp_poll_cb(void *arg, struct tcp_pcb *tpcb) {
	err_t ret_err;
	echo_raw_api_state_t* rs = (echo_raw_api_state_t*)arg;
	if (rs != NULL) {
		if (rs->packet_buffer != NULL) {
			// there is a remaining pbuf (chain)
			LWIPEchoCallback::__tcp_raw_send(tpcb, rs);
		} else {
			// no remaining pbuf (chain)
			if(rs->state == raw_api_status_e::CLOSING) {
				LWIPEchoCallback::__tcp_raw_close(tpcb, rs);
			}
		}
		ret_err = ERR_OK;
	} else {
		// nothing to be done
		tcp_abort(tpcb);
		ret_err = ERR_ABRT;
	}
	return ret_err;
}

// 关闭raw api，删除状态信息存储，关闭pcb
void LWIPEchoCallback::__tcp_raw_close(struct tcp_pcb* tpcb, echo_raw_api_state_t* rs) {
	info("destroy connection `%s:%d`->`%s:%d`, &tcp_pcb: %p", 
		LOCAL_IP(tpcb), LOCAL_PORT(tpcb), REMOTE_IP(tpcb), REMOTE_PORT(tpcb), tpcb);
	tcp_arg(tpcb, NULL);
	tcp_sent(tpcb, NULL);
	tcp_recv(tpcb, NULL);
	tcp_err(tpcb, NULL);
	tcp_poll(tpcb, NULL, 0);
	LWIPEchoCallback::__tcp_raw_state_free(rs);
	tcp_close(tpcb);
}

// 释放状态信息
void LWIPEchoCallback::__tcp_raw_state_free(echo_raw_api_state_t* rs) {
	if (rs != NULL) {
		if (rs->packet_buffer) {
			// free the buffer chain if present
			pbuf_free(rs->packet_buffer);
		}
		mem_free(rs);
	}
}

// 发送数据
void LWIPEchoCallback::__tcp_raw_send(struct tcp_pcb* tpcb, echo_raw_api_state_t* rs) {
	struct pbuf* ptr;
	err_t wr_err = ERR_OK;

	while ((wr_err == ERR_OK) && (rs->packet_buffer != NULL) && (rs->packet_buffer->len <= tcp_sndbuf(tpcb))) {
		ptr = rs->packet_buffer;
		// enqueue data for transmission
		wr_err = tcp_write(tpcb, ptr->payload, ptr->len, 1);
		if (wr_err == ERR_OK) {
			u16_t plen;
			plen = ptr->len;
			// continue with next pbuf in chain (if any)
			rs->packet_buffer = ptr->next;
			if(rs->packet_buffer != NULL) {
				// new reference!
				pbuf_ref(rs->packet_buffer);
			}
			// chop first pbuf from chain
			pbuf_free(ptr);
			// we can read more data now
			tcp_recved(tpcb, plen);
		} else if(wr_err == ERR_MEM) {
			// we are low on memory, try later / harder, defer to poll
			rs->packet_buffer = ptr;
		} else {
			/* other problem ?? */
		}
	}
}

void LWIPEchoCallback::udp_recv_cb(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port, const ip_addr_t *dest_addr, u16_t dest_port) {
	do {
		if (pcb == NULL) {
			break;
		}
		info("send udp packet from: `%s:%d`-->`%s:%d`", ipaddr_ntoa(addr), port, ipaddr_ntoa(dest_addr), dest_port);
		char buffer[MTU_USER_DEFINE];
		if(pbuf_copy_partial(p, buffer, p->tot_len,0) != p->tot_len) {
			error("pbuf_copy_partial() failed");
		} else {
			buffer[p->tot_len] = '\0';
			info("get message form `%s:%d`: %s", ipaddr_ntoa(addr), port, buffer);
		}
		// send received packet back to sender
		udp_sendto(pcb, p, addr, port, dest_addr, dest_port);
		// free the pbuf
		pbuf_free(p);
	} while (0);
}


