#include "logger.h"
#include "utun_device.h"

UTunDevice::UTunDevice(int tun_id, const char* ip, int mtu) {
	this->tun_id = tun_id;
	this->ip = ip;
	this->mtu = mtu;
	this->tun_fd = -1;
}

UTunDevice::~UTunDevice() {
	info("close tun device...");
	if (this->tun_fd > 0) {
		::close(this->tun_fd);
	}
}

int UTunDevice::open() {
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
	sprintf(cmd, "ifconfig utun%d inet %s %s mtu %d up", this->tun_id, this->ip, this->ip, this->mtu);

	info("exec: %s ...", cmd);
	ret = system(cmd);
	if (ret != 0) {
		info("exec command failed, err: %s", strerror(ret));
		return -1;
	}
	__set_non_block();
	return 0;
}

void UTunDevice::close() {
	info("close tun device...");
	if (this->tun_fd > 0) {
		::close(this->tun_fd);
		this->tun_fd = -1;
	}
}

int UTunDevice::get_fd() {
	assert(this->tun_fd > 0);
	return this->tun_fd;
}

// Protocol families are mapped onto address families
// --------------------------------------------------
// Notes:
//  - KEXT Controls and Notifications
//	  https://developer.apple.com/library/content/documentation/Darwin/Conceptual/NKEConceptual/control/control.html
int UTunDevice::__open_utun() {
	auto fd = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
	if (fd == -1) {
		error("socket(SYSPROTO_CONTROL) failed, abort!");
		return -1;
	}

	// set the socket non-blocking
	auto err = fcntl(fd, F_SETFL, O_NONBLOCK);
	if (err != 0) {
		error("set the socket non-blocking failed, abort!");
		::close(fd);
		return -1;	
	}

	// set close-on-exec flag
	auto flags = fcntl(fd, F_GETFD);
	flags |= FD_CLOEXEC;
	err = fcntl(fd, F_SETFD, flags);
	if (err != 0) {
		error("set close-on-exec flag failed, abort!");
		::close(fd);
		return -1;
	}

	// Convert a kernel control name to a kernel control id
	// --------------------------------------------------
	// Notes:
	//  - CTLIOCGINFO
	//	  https://developer.apple.com/documentation/kernel/ctliocginfo?language=objc
	//  - strlcpy
	//	  https://en.wikipedia.org/wiki/C_string_handling
	ctl_info ci{};

	if (strlcpy(ci.ctl_name, UTUN_CONTROL_NAME, sizeof(ci.ctl_name)) >= sizeof(ci.ctl_name)) {
		error("UTUN_CONTROL_NAME is too long, abort!");
		::close(fd);
		return -1;	
	}

	if (ioctl(fd, CTLIOCGINFO, &ci) == -1) {
		error("ioctl(CTLIOCGINFO) failed, abort!");
		::close(fd);
		return -1;
	}
	
	sockaddr_ctl sc{};
	sc.sc_id		= ci.ctl_id;
	sc.sc_len	   	= sizeof(sc);
	sc.sc_family	= AF_SYSTEM;
	sc.ss_sysaddr	= AF_SYS_CONTROL;
	sc.sc_unit	  	= this->tun_id+1;

	if (connect(fd, (struct sockaddr *)&sc, sizeof(sc)) == -1) {
		error("connect(AF_SYS_CONTROL) failed, abort!");
		::close(fd);
		return -1;
	}
	this->tun_fd = fd;
	return 0;
}

void UTunDevice::__set_non_block(bool enable) {
	assert(this->tun_fd>0);

    int flags = fcntl(this->tun_fd, F_GETFL, 0);
    if (flags < 0 || fcntl(this->tun_fd, F_SETFL, enable ? flags | O_NONBLOCK : flags & ~O_NONBLOCK) < 0) {
        error("set fd non block failed, err: %s\n", strerror(errno));
    }
}
