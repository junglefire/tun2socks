#ifndef TUN2SOCKS_CONN_SESSION_H__
#define TUN2SOCKS_CONN_SESSION_H__

#include <lwip/tcp.h>

#include <arpa/inet.h>
#include <stdlib.h>
#include <string>
#include <map>

#include "logger.h"

#define SESSION (ConnSession::get_instance())

// std::map的key需要提供operator<()
typedef struct _session_key_t {
	u32_t s_ip;
	u16_t s_port;
	u32_t d_ip;
	u16_t d_port;

	bool operator< (const _session_key_t& x) const {
		if (s_ip < x.s_ip) {
			return true;
		}
		if (s_ip == x.s_ip && s_port < x.s_port) {
			return true;
		}
		if (s_ip == x.s_ip && s_port == x.s_port && d_ip < x.d_ip) {
			return true;
		}
		if (s_ip == x.s_ip && s_port == x.s_port && d_ip == x.d_ip && d_port < x.d_port) {
			return true;
		}
		return false;
	}
} session_key_t;

typedef struct _session_str_key_t {
	std::string s_ip;
	u16_t s_port;
	std::string d_ip;
	u16_t d_port;

	bool operator< (const _session_str_key_t& x) const {
		if (s_ip < x.s_ip) {
			return true;
		}
		if (s_ip == x.s_ip && s_port < x.s_port) {
			return true;
		}
		if (s_ip == x.s_ip && s_port == x.s_port && d_ip < x.d_ip) {
			return true;
		}
		if (s_ip == x.s_ip && s_port == x.s_port && d_ip == x.d_ip && d_port < x.d_port) {
			return true;
		}
		return false;
	}
} session_str_key_t;

// Scott Meyers中的提出另一种更优雅的单例模式实现，使用local static对象
// C++0X以后，要求编译器保证内部静态变量初始化的线程安全，所以C++0x之后该实现是线程安全的
class ConnSession {
public:
	// 注意返回的是引用
	static ConnSession& get_instance()
	{
		static ConnSession instance;  // 静态局部变量
		return instance;
	}

	bool insert(u32_t, u16_t, u32_t, u16_t, void*);
	bool remove(u32_t, u16_t, u32_t, u16_t);
	ConnSession() = default;
	~ConnSession();
	ConnSession(const ConnSession&) = delete;
	ConnSession& operator=(const ConnSession&) = delete;
public:
	void show_all_session();
private:
	std::string session_string(session_key_t sess);
private:
	std::map<session_key_t, void*> tcp_session;
	std::map<session_key_t, void*> udp_session;
};

#endif // TUN2SOCKS_CONN_SESSION_H__