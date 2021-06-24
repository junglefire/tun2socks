#include "lwip_timer.h"

LWIPTimer::LWIPTimer(uv_loop_t* loop) {
	this->loop = loop;
}

LWIPTimer::~LWIPTimer() {
}

int LWIPTimer::start() {
	uv_timer_init(loop, &this->timeout_handler);
	uv_timer_start(&this->timeout_handler, LWIPTimer::check_timeouts, CHECK_TIMEOUTS_INTERVAL, CHECK_TIMEOUTS_INTERVAL);
	return 0;
}

void LWIPTimer::restart() {
	sys_restart_timeouts();
}

void LWIPTimer::check_timeouts(uv_timer_t* handle){
	// debug("timeout");
	sys_check_timeouts();
}




