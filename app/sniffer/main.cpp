#include <sys/types.h>
#include <getopt.h>
#include <stdlib.h>
#include <string>

#include "sniffer.h"
#include "logger.h"

#define ZLOG_CONF "etc/zlog.conf"

/*********************************************************************************/
/**																			 	**/
/** sniffer																	 	**/ 
/** - 使用libpcap构造的网卡监听工具													**/ 
/**																			 	**/ 
/*********************************************************************************/

#define LONG_ARG_NUMBER 0

// 命令行参数
typedef struct _args {
	const char* interface;
	int number;
	const char* written_file;
	std::string rule;
	bool verbose;
} args_t;

// 函数定义
int parse_args(int, char**, args_t&); 

// 主函数
int main(int argc, char* argv[]) {
	// 初始化参数列表
	args_t args = {
		nullptr,	// 指定网卡，如果没有指定，自动获取默认的网卡
		-1,			// 抓包数量，-1表示持续抓包
		nullptr,	// 结果输出到文件(PCAP格式)
		"",			// BPF过滤规则
		false, 		// 显示详细包信息
	};	
	
	int ret = parse_args(argc, argv, args);
	if (ret != 0) {
		return -1;
	}

#ifdef ZLOG_MESSAGE
	ret = dzlog_init(ZLOG_CONF, "sniffer");
	if (ret != 0) {
		fprintf(stderr, "zlog load configuration file `%s` failed\n", ZLOG_CONF);
		return -1;
	}
#endif //ZLOG_MESSAGE

	try {
		Sniffer* sniffer = &Sniffer::instance(args.interface);
		// 设置输出文件
		if (args.written_file != nullptr) {
			sniffer->set_written_file(args.written_file);
		}
		// 设置最大抓包数
		sniffer->set_max_num_of_packet(args.number); 
		// 设置BPF规则
		if (!args.rule.empty()) {
			sniffer->set_bpf_rule(args.rule);
		}
		sniffer->set_verbose(args.verbose);
		sniffer->run();
	} catch (const char* msg) {
		error("Exception: %s", msg);
		return -1;
   	}
	return 0;
}

// 命令行参数解析
int parse_args(int argc, char* argv[], args_t& args) {
	std::string usage = "[Usage] sniffer [-i|--interface interface] [--number] [-w|--written-file file] [-v|--verbose]";
	const char* optstr = "i:w:c:vh";

	struct option opts[] = {
		{"interface", 1, NULL, 'i'},
		{"max-packet", 1, NULL, 'c'},
		{"written-file", 1, NULL, 'w'},
		{"verbose", 1, NULL, 'v'},
		{"help", 0, NULL, 'h'},
		{0, 0, 0, 0},
	};
	
	int opt;
	while((opt = getopt_long(argc, argv, optstr, opts, NULL)) != -1){
		switch(opt) {
		case 'i': // 指定网卡，如果没有指定，自动获取默认的网卡
			args.interface = optarg;
			break;
		case 'c':
			args.number = std::atoi(optarg);
			break;
		case 'v':
			args.verbose = true;
			break;
		case 'w': // 结果输出到文件(PCAP格式)
			args.written_file = optarg;
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

	// 如果指定了BPF过滤规则
	if (optind < argc) {
		int i = optind;
		while (i < argc) {
			args.rule += argv[i];
			args.rule += " ";
			i+=1;
		}
	}
	return 0;
}




