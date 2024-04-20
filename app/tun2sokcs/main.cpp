#include <sys/types.h>
#include <getopt.h>
#include <stdlib.h>

#include "conn_session.h"
#include "utun_device.h"
#include "tun_device.h"
#include "lwip_timer.h"
#include "lwip_stack.h"
#include "pipe2socks.h"
#include "tun2socks.h"
#include "nat_table.h"
#include "common.h"
#include "logger.h"

#define ZLOG_CONF "etc/zlog.conf"

/*********************************************************************************/
/**																			 	**/
/** tun2socks																 	**/ 
/** - 使用LWIP																	**/ 
/**																			 	**/ 
/*********************************************************************************/

// 命令行参数
typedef struct _args {
	const char* device;
	int tun_id;
	const char* tun_ip;
	int mtu;
	const char* nat_table;
	const char* service_type;
} args_t;

// 函数定义
int parse_args(int, char**, args_t&); 
int open_tun_device(int, const char*, int);
int start_tun2socks(const args_t&);
int start_pipe2socks(const args_t&);
void remove_pipe_signal_handler(int);
void set_non_block(int, bool);

// 主函数
int main(int argc, char* argv[]) {
	int ret = 0;
	int tun_fd;

	// 初始化参数列表
	args_t args = {
		"tun",			// device type, include `tun` and `pipe`
		10,				// tun device id 
		"10.25.0.1",	// tun ip
		MTU_USER_DEFINE,// mtu
		"etc/nat.txt",	// nat table
		"proxy"			// service mode, such as `echo`, `proxy`, etc.
	};	
	
	ret = parse_args(argc, argv, args);
	if (ret != 0) {
		return -1;
	}

#ifdef ZLOG_MESSAGE
	ret = dzlog_init(ZLOG_CONF, "tun2socks");
	if (ret != 0) {
		fprintf(stderr, "zlog load configuration file `%s` failed\n", ZLOG_CONF);
		return -1;
	}
#endif //ZLOG_MESSAGE

	if (std::strcmp(args.device, "tun") == 0) {
		info("bin/tun2socks --device %s --tun-id %d --tun-ip %s --mtu %d --nat %s --service-type %s", 
			args.device, args.tun_id, args.tun_ip, args.mtu, args.nat_table, args.service_type);
		start_tun2socks(args);
	} else if (std::strcmp(args.device, "pipe") == 0) {
		info("bin/tun2socks --device %s --mtu %d --nat %s --service-type %s", 
			args.device, args.mtu, args.nat_table, args.service_type);
		start_pipe2socks(args);
	} else {
		error("invalid device type, abort!");
		return -1;
	}
	return 0;
}

// 启动tun2socks服务
int start_tun2socks(const args_t& args) {
	info("start tun2socks service...");

	// IDevice* tun = new XTunDevice(args.tun_id, args.tun_ip, args.mtu);
	std::unique_ptr<IDevice> tun(new UTunDevice(args.tun_id, args.tun_ip, args.mtu));
	// std::unique_ptr<IDevice> tun(new TunDevice(args.tun_id, args.tun_ip, args.mtu));
	tun->open();

#ifdef __TEST_TUN2SOCKS__2
	info("route add -net 172.168.0.1 -netmask 255.255.255.0 10.25.0.1");
	system("route add -net 172.168.0.1 -netmask 255.255.255.0 10.25.0.1");
#endif
	
	// 判断是否需要加载NAT映射表
	if (args.nat_table) {
		info("load nat table `%s`...", args.nat_table);
		NatTable* nt = NatTable::get_instance();
		int n = nt->load(args.nat_table);
		if (n>0) {
			info("load `%d` nat rules...", n);
		} else {
			error("load nat rules failed, abort");
			return -1;
		}
	}

	uv_loop_t* LOOP = uv_default_loop();

	// LWIP协议栈初始化
	LWIPStack lwip;
	if (lwip.init(args.service_type) !=0 ) {
		error("LWIPStack init failed, abort!");
		return -1;
	}
	// 超时事件处理
	LWIPTimer lt(LOOP);
	lt.start();

	// tun事件处理
	Tun2Socks ts(tun->get_fd(), LOOP, lwip);
	ts.start();
	
	// 启动uvloop
	info("uvloop run...");
	uv_run(LOOP, UV_RUN_DEFAULT);
	uv_loop_delete(LOOP);	
	return 0;
}

// 启动tun2socks服务
int start_pipe2socks(const args_t& args) {
	info("start pipe2socks service...");

	// 判断是否需要加载NAT映射表
	if (args.nat_table) {
		info("load nat table `%s`...", args.nat_table);
		NatTable* nt = NatTable::get_instance();
		int n = nt->load(args.nat_table);
		if (n>0) {
			info("load `%d` nat rules...", n);
		} else {
			error("load nat rules failed, abort");
			return -1;
		}
	}

	uv_loop_t* LOOP = uv_default_loop();

	// LWIP协议栈初始化
	LWIPStack lwip;
	if (lwip.init(args.service_type) !=0 ) {
		error("LWIPStack init failed, abort!");
		return -1;
	}

	// 创建PIPE
	Pipe2Socks& ps = Pipe2Socks::get_pipe2socks_singleton(PIPENAME, LOOP, lwip);
	if (ps.start() != 0) {
		error("pipe2socks start failed, abort!");
		return -1;
	}

	signal(SIGINT, remove_pipe_signal_handler);

	// 超时事件处理
	LWIPTimer lt(LOOP);
	lt.start();

	// 启动uvloop
	info("uvloop run...");
	uv_run(LOOP, UV_RUN_DEFAULT);
	uv_loop_delete(LOOP);
	return 0;
}

void remove_pipe_signal_handler(int sig) {
	uv_fs_t req;
	// 删除unix域对应的路径
	info("remove named pipe `%s`", PIPENAME);
	uv_fs_unlink(uv_default_loop(), &req, PIPENAME, NULL);
	exit(0);
}

// 命令行参数解析
int parse_args(int argc, char* argv[], args_t& args) {
	std::string usage = "[Usage] tun2socks --device pipe --mtu <number> --nat <nat table> --service-type <echo|proxy>\n";
	usage += "[Usage] tun2socks --device tun --tun-id <id> --tun-ip <ip> --mtu <number> --nat <nat table> --service-type <echo|proxy>";
	const char* optstr = "v:d:p:m:n:t:h";

	struct option opts[] = {
		{"device", 1, NULL, 'v'},
		{"tun-id", 1, NULL, 'd'},
		{"tun-ip", 1, NULL, 'p'},
		{"mtu", 1, NULL, 'm'},
		{"nat", 1, NULL, 'n'},
		{"service-type", 1, NULL, 't'},
		{"help", 0, NULL, 'h'},
		{0, 0, 0, 0},
	};
	
	int opt;
	while((opt = getopt_long(argc, argv, optstr, opts, NULL)) != -1){
		switch(opt) {
		case 'v':
			args.device = optarg;
			break;
		case 'd':
			args.tun_id = atoi(optarg);
			break;
		case 'p':
			args.tun_ip = optarg;
			break;
		case 'n':
			args.nat_table = optarg;
			break;
		case 't':
			args.service_type = optarg;
			break;
		case 'm':
			args.mtu = atoi(optarg);
			break;
		case 'h':
			fprintf(stderr, "%s\n", usage.c_str());
			return -1;
		case '?':
			if(strchr(optstr, optopt) == NULL){
				fprintf(stderr, "unknown option '-%c'\n", optopt);
			}else{
				fprintf(stderr, "option requires an argument '-%c'\n", optopt);
			}
			return -1;
		}
	}
	return 0;
}




