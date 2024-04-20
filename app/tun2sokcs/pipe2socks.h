#ifndef PIPE2SOCKS_EVENT_H__
#define PIPE2SOCKS_EVENT_H__

/*********************************************************************************/
/**																			 	**/
/** Unix Domain Socket事件注册/注销												**/
/** 																			**/
/*********************************************************************************/
#include <sys/types.h>
#include <sys/uio.h>
#include <stdlib.h>
#include <assert.h>
#include <uv.h>

#include <fstream>
#include <string>
#include <map>

#include "lwip_stack.h"
#include "common.h"
#include "logger.h"

typedef struct {
	uv_write_t req;
	uv_buf_t buf;
} write_req_t;

class Pipe2Socks {
public:
	static Pipe2Socks& get_pipe2socks_singleton(std::string pipe_name, uv_loop_t* loop, LWIPStack& lwip) {
    	static Pipe2Socks s(pipe_name, loop, lwip);
    	return s;
  	}

	Pipe2Socks(const Pipe2Socks&) = delete;
	Pipe2Socks& operator=(const Pipe2Socks&) = delete;
	int start();
private:
	Pipe2Socks(std::string, uv_loop_t*, LWIPStack& lwip);
	~Pipe2Socks();
	void pipe_client_connect_cb();
	void pipe_read_and_write_lwip_cb(ssize_t, const uv_buf_t*);
	void pipe_read_and_echo_cb(ssize_t, const uv_buf_t*);
	void write_pipe();
	static void alloc_buffer(uv_handle_t*, size_t, uv_buf_t*);
	static void free_write_req(uv_write_t*);
	static err_t output_tun(struct pbuf *p);
	static err_t __output_tun(void*, u16_t);
private:
	static uv_stream_t* __client_ptr;
	static uv_write_t* __write_ptr;
	std::string pipe_name;
	uv_loop_t* loop;
	uv_pipe_t server;
	uv_pipe_t client;
	uv_write_t write_req;
	LWIPStack& lwip;
};

#endif // PIPE2SOCKS_EVENT_H__