#ifndef TUN2SOCKS_TUN_EVENT_H__
#define TUN2SOCKS_TUN_EVENT_H__

/*********************************************************************************/
/**																			 	**/
/** Tun Device事件注册/注销														**/
/** 																			**/
/*********************************************************************************/
#include <sys/types.h>
#include <sys/uio.h>
#include <stdlib.h>
#include <assert.h>
#include <uv.h>

#include <string>
#include <map>

#include "lwip_stack.h"
#include "tun_device.h"
#include "logger.h"

#define STOP 	0
#define RUNNING	1

#define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))

// 会话状态码
typedef enum _connect_status_enum {
	CONNECTING,
	CONNECTED
} connect_status_enum;

// 会话信息
// todo：先以string做key，后续再优化
typedef struct _session {
	uv_connect_t* uv_conn;
	struct tcp_pcb* tcp_pcb;
	connect_status_enum status;
} session_t;

// todo: 把tun device、LWIP、UV分别抽象出来，在Tun2Socks组装
class Tun2Socks {
public:
	Tun2Socks(int, uv_loop_t*, LWIPStack& lwip);
	~Tun2Socks();
	int start();
private:
	void setup_output_tun_cb();
	static void on_poll_cb(uv_poll_s*, int, int);
	static err_t output_tun(struct pbuf *p);
	static err_t __output_tun(void*, u16_t);
public:
	static int tun_output_fd;
	// 会话存储表
	// static std::map<std::string, session_t> sessions;
private:
	int tun_fd;
	uv_loop_t* loop;
	uv_poll_t* poll;
	int status;
	LWIPStack& lwip;
private:
	Tun2Socks(const Tun2Socks&);
	Tun2Socks& operator=(const Tun2Socks&);
};

#endif // TUN2SOCKS_TUN_EVENT_H__