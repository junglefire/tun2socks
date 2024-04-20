#ifndef TUN2SOCKS_DEVICE_H__
#define TUN2SOCKS_DEVICE_H__

class IDevice
{
public:
	virtual int open() = 0;
	virtual void close() = 0;
	virtual int get_fd() = 0;
};

#endif // TUN2SOCKS_DEVICE_H__