#include "logger.h"
#include "tun_device.h"

TunDevice::TunDevice(int tun_id, const char* ip, int mtu) {
	this->tun_id = tun_id;
	this->ip = ip;
	this->mtu = mtu;
	this->tun_fd = -1;
}

TunDevice::~TunDevice() {
	info("close tun device...");
	if (this->tun_fd > 0) {
		::close(this->tun_fd);
	}
}

int TunDevice::open() {
	info("open tun device `%d`, set ip `%s` and mtu `%d`", this->tun_id, this->ip, this->mtu);

	int ret = this->__open_utun();
	if (ret >= 0) {
		info("A tun interface has been created");
	} else {
		error("create tun failed, abort!");
		return -1;
	}

	char cmd[128];
	memset(cmd, 0, 128);
	sprintf(cmd, "ifconfig tun%d inet %s %s mtu %d up", this->tun_id, this->ip, this->ip, this->mtu);

	info("exec: %s ...", cmd);
	ret = system(cmd);
	if (ret != 0) {
		info("exec command failed, err: %s", strerror(ret));
		return -1;
	}
	// __set_non_block();
	return 0;
}

void TunDevice::close() {
	info("close tun device...");
	if (this->tun_fd > 0) {
		::close(this->tun_fd);
		this->tun_fd = -1;
	}
}

int TunDevice::get_fd() {
	assert(this->tun_fd > 0);
	return this->tun_fd;
}

int TunDevice::__open_utun() {
	std::string tun_name = "/dev/tun" + std::to_string(this->tun_id);
	auto fd = ::open(tun_name.c_str(), O_RDWR|O_NONBLOCK);
	if (fd == -1) {
		error("open tun device failed, abort!");
		return -1;
	}
	this->tun_fd = fd;
	return 0;
}

void TunDevice::__set_non_block(bool enable) {
	assert(this->tun_fd>0);

    int flags = fcntl(this->tun_fd, F_GETFL, 0);
    if (flags < 0 || fcntl(this->tun_fd, F_SETFL, enable ? flags | O_NONBLOCK : flags & ~O_NONBLOCK) < 0) {
        error("set fd non block failed, err: %s\n", strerror(errno));
    }
}
