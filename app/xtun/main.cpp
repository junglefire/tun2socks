#include <sys/types.h>
#include <sys/time.h>
#include <getopt.h>
#include <stdlib.h>
#include <string>
#include <ctime>

#include <utun_device.h>
#include <logger.h>

#define ZLOG_CONF 	"etc/zlog.conf"
#define __CHECK__ 	1
#define MTU 		1500

/*********************************************************************************/
/**																			 	**/
/** xtun																	 	**/ 
/**   - tun devie I/O on MacOSX													**/ 
/**																			 	**/ 
/*********************************************************************************/

// 命令行参数
typedef struct _args {
	bool mt;
} args_t;

// 主函数
int main(int argc, char* argv[]) {
	std::unique_ptr<IDevice> tun(new UTunDevice(10, "10.25.0.1", MTU));
	tun->open();
	char buffer[MTU];
	for (int i=0; i<10; i++) {
		ssize_t length = read(tun->get_fd(), buffer, MTU);
		info("read %d bytes to tun device, err: %s", length, strerror(errno));
		int wlen = write(tun->get_fd(), "alex", 4);
		info("write %d bytes to tun device", wlen);
		sleep(3);
	}
	return 0;
}






