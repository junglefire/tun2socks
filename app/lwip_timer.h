#ifndef TUN2SOCKS_LWIP_TIMER_H__
#define TUN2SOCKS_LWIP_TIMER_H__

#include <lwip/timeouts.h>
#include <lwip/init.h>
#include <lwip/tcp.h>
#include <lwip/udp.h>
#include <lwip/sys.h>
#include <uv.h>

#include "logger.h"

#define CHECK_TIMEOUTS_INTERVAL 250  // in millisecond

class LWIPTimer {
public:
	LWIPTimer(uv_loop_t*);
	~LWIPTimer();
	int start();
	void restart();
public:
	static void check_timeouts(uv_timer_t*);
private:
	uv_loop_t* loop;
	uv_timer_t timeout_handler;
private:
	LWIPTimer(const LWIPTimer&);
	LWIPTimer& operator=(const LWIPTimer&);
};


#endif // TUN2SOCKS_LWIP_TIMER_H__