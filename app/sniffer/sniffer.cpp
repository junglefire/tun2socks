#include "sniffer.h"

Sniffer::Sniffer(const char* device, const char* rule) {
	if (device != nullptr) {
		this->device = device;
	} else {
		if (this->get_default_device() != 0) {
			throw this->errmsg;
		} 
	}
	this->dumper = nullptr;
	this->num_of_packet = -1;
	this->verbose = false;
}

Sniffer::~Sniffer() {
}

int Sniffer::run() {
	info("listen on device `%s` ...", this->device.c_str());

	int ret = 0;
	this->pcap_handler = pcap_open_live(this->device.c_str(), BUFSIZ, 1, 1000, this->errmsg);
	if (this->pcap_handler == nullptr) {
		error("couldn't open device `%s`, err: %s", this->device.c_str(), this->errmsg);
		return -1;
	}

	this->num_of_packet = 0;

	// 是否写入PCAP文件
	if (!this->written_file.empty()) {
		this->dumper = pcap_dump_open(this->pcap_handler, this->written_file.c_str());
		if (this->dumper == nullptr) {
			error("open pcap dumper failed, err: %s",  pcap_geterr(this->pcap_handler));
			return -1;
		}
	}

	// 设置BPF规则
	struct bpf_program filter;
	if (!this->rule.empty()) {
		ret = pcap_compile(this->pcap_handler, &filter, this->rule.c_str(), 1, 0);
		if (ret != 0) {
			error("compile BPF rule failed, err: %s", pcap_geterr(this->pcap_handler));
			return -1;
		}

		ret = pcap_setfilter(this->pcap_handler, &filter);
		if (ret != 0) {
			error("set BPF filter failed, err: %s", pcap_geterr(this->pcap_handler));
			return -1;
		}
	}
	
	// 循环抓包
	pcap_loop(this->pcap_handler, this->max_num_of_packet, [](u_char* arg, const struct pcap_pkthdr* pkthdr, const u_char* packet){ 
		Sniffer* s = (Sniffer*)arg;	
		info("packet count: %d", (s->num_of_packet)++);
		if (!s->verbose) {
			s->print_summary_info(pkthdr, packet);
		} else {
			s->print_detailed_info(pkthdr, packet);
		}
		return;
	}, (u_char*)this);
	
	// 刷新输出文件/关闭输出文件
	if (!this->written_file.empty()) {
		pcap_dump_flush(this->dumper);
		pcap_dump_close(this->dumper);
	}
	// 关闭
	pcap_close(this->pcap_handler);
	return 0;
}

Sniffer& Sniffer::instance(const char* device, const char* rule) {
	static Sniffer s(device, rule);
	return s;
}

void Sniffer::set_written_file(const char* filename, bool backup) {
	this->written_file = filename;
	std::ifstream f(filename);

	// 是否备份旧文件
	if (!backup) {
		return;
	}

	if (f.good()) {
		std::time_t now = std::time(0);
		std::string new_filename(filename);
		new_filename += "_";
		new_filename += std::to_string(now);
		info("written file exist, backup!");
		std::rename(filename, new_filename.c_str());
		info("rename file `%s` to `%s`", filename, new_filename.c_str());
	}
}

void Sniffer::set_verbose(bool verbose) {
	this->verbose = verbose;
}

void Sniffer::set_max_num_of_packet(int max) {
	this->max_num_of_packet = max;
}

void Sniffer::set_bpf_rule(std::string rule) {
	this->rule = rule;
}

// 获取缺省的网卡
int Sniffer::get_default_device() {
	char *dev;
	std::memset(this->errmsg, 0, sizeof(this->errmsg));

	info("get default device...");

	dev = pcap_lookupdev(this->errmsg);
	if (dev == nullptr) {
		return -1;
	}

	this->device = dev;
	return 0;
}

void Sniffer::print_summary_info(const struct pcap_pkthdr* pkthdr, const u_char* packet) {
	info("recv packet size: %d", pkthdr->len);
	pcap_dump((u_char*)this->dumper, pkthdr, packet);
}

void Sniffer::print_detailed_info(const struct pcap_pkthdr* pkthdr, const u_char* packet) {
	info("Recv Packet Size: %d", pkthdr->len);

	const struct sniff_ethernet* ethernet;
	const struct sniff_ip* ip;
	const struct sniff_tcp* tcp;
	const u_char* payload;

	u_int size_ip;
	u_int size_tcp;

	ethernet = (struct sniff_ethernet*)(packet);
	ip = (struct sniff_ip*)(packet + SIZE_ETHERNET);
	size_ip = IP_HL(ip)*4;
	if (size_ip < 20) {
		error("invalid IP header length: %u bytes", size_ip);
		return;
	}

	tcp = (struct sniff_tcp*)(packet + SIZE_ETHERNET + size_ip);
	size_tcp = TH_OFF(tcp)*4;
	if (size_tcp < 20) {
		error("invalid TCP header length: %u bytes", size_tcp);
		return;
	}

	info("%s:%d -> %s:%d", inet_ntoa(ip->ip_src), tcp->th_sport, inet_ntoa(ip->ip_dst), tcp->th_dport);

	payload = (u_char*)(packet + SIZE_ETHERNET + size_ip + size_tcp);
	info("payload: %s", payload);
	// pcap_dump((u_char*)this->dumper, pkthdr, packet);
}





