#ifndef TUN2SOCKS_DEVICE_H__
#define TUN2SOCKS_DEVICE_H__

class IPlugin
{
public:
	virtual int init() = 0;
	virtual void finit() = 0;
	virtual int process() = 0;
};

#endif // TUN2SOCKS_DEVICE_H__