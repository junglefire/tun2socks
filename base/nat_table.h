#ifndef TUN2SOCKS_NAT_TABLE_H__
#define TUN2SOCKS_NAT_TABLE_H__

#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <map>

#include "logger.h"

// 一个线程不安全的单例类
class NatTable {
public:
	static NatTable* get_instance();
	int load(const char*);
	const char* query(const char*);
	bool is_nat_active();
private:
	static NatTable* _singleton;
	int count_of_rules;
	std::map<std::string, std::string> table;
protected:
	NatTable();
	~NatTable();
private:
	NatTable(const NatTable&);
	NatTable& operator=(const NatTable&);
};


#endif // TUN2SOCKS_NAT_TABLE_H__