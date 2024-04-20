#ifndef TUN2SOCKS_XTUN_DEVICE_H__
#define TUN2SOCKS_XTUN_DEVICE_H__

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include <net/if_utun.h>
#include <net/if.h>

#include <sys/kern_control.h>
#include <sys/kern_event.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/errno.h>

#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>

#include "logger.h"
#include "device.h"

#define IP4_HDRLEN 20		 // IPv4 header length
#define MTU 1500

// 封装MacOS的Tun设备
class UTunDevice: public IDevice
{
public:
	UTunDevice(int, const char*, int mtu=MTU);
	~UTunDevice();
	
	int open();
	void close();
	int get_fd();

	UTunDevice& operator=(const UTunDevice&) = delete;
	UTunDevice(const UTunDevice&) = delete;
private:
	int __open_utun(); 
	void __set_non_block(bool enable=true); 
public:
	int tun_id;
	int tun_fd;
	const char* ip;
	int mtu;
};

#endif // TUN2SOCKS_XTUN_DEVICE_H__