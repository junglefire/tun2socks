#ifndef SINFFER_ALEX_H__
#define SINFFER_ALEX_H__

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <regex>
#include <ctime>
#include <map>

#include <pcap.h>

#include "logger.h"
#include "packet.h"

inline std::string timeToString(std::chrono::system_clock::time_point &t)
{
	std::time_t time = std::chrono::system_clock::to_time_t(t);
	std::string time_str = std::ctime(&time);
	time_str.resize(time_str.size() - 1);
	return time_str;
}

// 一个线程不安全的单例类
class Sniffer {
public:
	// 禁止拷贝构造和赋值
	Sniffer(const Sniffer&);
	Sniffer& operator=(const Sniffer&);
	// Meyer's Singleton
	static Sniffer& instance(const char* device = nullptr, const char* rule = nullptr);
	// 开始抓包
	int run();
	// 设置输出文件名称
	void set_written_file(const char* filename, bool backup=false);
	// 设置抓包个数
	void set_max_num_of_packet(int max=-1);
	// 设置BPF规则
	void set_bpf_rule(std::string);
	// 设置是否显示包头详细信息
	void set_verbose(bool);
private:
	// 获取缺省的网卡
	int get_default_device();
	// 打印概要信息
	void print_summary_info(const struct pcap_pkthdr*, const u_char*);
	// 打印详细信息
	void print_detailed_info(const struct pcap_pkthdr*, const u_char*);
private:
	bool verbose;						// 是否显示包头信息
	std::string device;					// 网卡名称
	std::string rule;					// BPF规则
	char errmsg[PCAP_ERRBUF_SIZE];		// PCAP错误信息
	pcap_t* pcap_handler;				// PCAP句柄
	pcap_dumper_t* dumper;				// PCAP文件写入句柄
	int num_of_packet;					// 捕捉的包个数
	int max_num_of_packet;				// 捕捉包的上限，-1表示没有上限
	std::string written_file;			// 将包内容写入PCAP文件
private:
	Sniffer(const char*, const char*);
	~Sniffer();
};

#endif // SINFFER_ALEX_H__