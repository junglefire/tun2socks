#include "conn_session.h"

ConnSession::~ConnSession() {
}

bool ConnSession::insert(u32_t s_ip, u16_t s_port, u32_t d_ip, u16_t d_port, void* value) {
	info("add session: `%s:%d`-->`%s:%d`, &proxy: %p", 
		inet_ntoa(in_addr{s_ip}), s_port, 
		inet_ntoa(in_addr{d_ip}), d_port, value);

	session_key_t key{
		s_ip, 
		s_port, 
		d_ip, 
		d_port
	};

	auto [_, ret] = this->tcp_session.insert(std::map<session_key_t, void*>::value_type(key, value));
	return ret;
}

bool ConnSession::remove(u32_t s_ip, u16_t s_port, u32_t d_ip, u16_t d_port) {
	info("remove session: `%s:%d`-->`%s:%d`", 
		inet_ntoa(in_addr{s_ip}), s_port, 
		inet_ntoa(in_addr{d_ip}), d_port);

	session_key_t key{
		s_ip, 
		s_port, 
		d_ip, 
		d_port
	};

	auto ret = this->tcp_session.erase(key);
	info("remove %d item", ret);
	return ret;
}

void ConnSession::show_all_session() {
	for (const auto& [key, value]: this->tcp_session) {
		info(" * %s -> %p", this->session_string(key).c_str(), value);
	}
}

std::string ConnSession::session_string(session_key_t sess) {
	char temp[64];
	// snprintf(temp, 64, "%u:%d->%u:%d", sess.s_ip, sess.s_port, sess.d_ip, sess.d_port);
	snprintf(temp, 64, "%s:%d->%s:%d", 
		inet_ntoa(in_addr{sess.s_ip}), sess.s_port, 
		inet_ntoa(in_addr{sess.d_ip}), sess.d_port);
	return std::string(temp);
}






