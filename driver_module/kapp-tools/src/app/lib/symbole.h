#ifndef _KSYS_LIB_SYSMBOLE_H_
#define _KSYS_LIB_SYSMBOLE_H_

#include <string>
#include <set>
#include <map>
#include <vector>
#include <algorithm>
#include <iostream>
#include <string.h>
#include <algorithm>

#define INVALID_ADDR ((size_t)(-1))
struct fsymbol {
	size_t start;
	size_t end;
	size_t ip;
	std::string name;

	fsymbol() :start(0), end(0), ip(0) {}
	fsymbol(size_t pc) :start(0), end(0), ip(pc) {}

	void reset(size_t va) { start = end = 0; ip = va; }
	bool operator< (const fsymbol &sym) const {
		return sym.ip < start;
	}

	bool operator> (const fsymbol &sym) const {
		return sym.ip > end;
	}
};


class elf_symbol {

private:
	std::set<fsymbol> kernel_symbols;

	bool load_kern(void);
public:
	bool find_kernel_symbol(fsymbol &sym);
	bool symbol_init(void);

	elf_symbol();
	~elf_symbol();
};

extern class elf_symbol *g_elf_sym;
#endif
