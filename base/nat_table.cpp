#include "nat_table.h"

NatTable* NatTable::_singleton = nullptr;;

NatTable::NatTable() {
	this->count_of_rules = 0;
}

NatTable::~NatTable() {
	delete _singleton;
}

NatTable* NatTable::get_instance() {
	if(_singleton == nullptr){
		_singleton = new NatTable();
	}
	return _singleton;
}

int NatTable::load(const char* filename) {
	std::ifstream ifile;
	std::string line;
	std::regex re("(.*)\\s:\\s(.*)");
	std::smatch sm;
	ifile.open(filename, std::ios::in);
	while (std::getline(ifile, line)) {
		std::regex_search(line, sm, re);
		info("nat rule: %s -> %s", sm[1].str().c_str(), sm[2].str().c_str());
		this->table[sm[1]] = sm[2];
		this->count_of_rules++;
	}
	return this->count_of_rules;
}

const char* NatTable::query(const char* ip) {
	if (this->table.find(ip) != this->table.end()) {
		return this->table[ip].c_str();
	} else {
		return "";
	}
}

bool NatTable::is_nat_active() {
	return this->count_of_rules != 0;
}





