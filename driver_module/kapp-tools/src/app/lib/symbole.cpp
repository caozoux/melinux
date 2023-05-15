#include "symbole.h"
#include <iostream>
#include <ostream>

bool is_space(int ch) {
    return std::isspace(ch);
}

static inline void rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), is_space).base(), s.end());
}

static bool get_next_kernel_symbol(
        std::set<fsymbol> &syms,
        std::vector<std::string> &sym_list,
        std::vector<std::string>::iterator cursor)
{
    if (cursor == sym_list.end()) {
        return false;
    }
    fsymbol sym;
    size_t start, end;
    sscanf(cursor->c_str(), "%p %*c %*s\n", (void **)&start);
    sym.name = cursor->c_str() + 19;
    rtrim(sym.name);
#if 0
    if (sym.name[sym.name.size()-1] == '\n') {
        sym.name[sym.name.size()-1] = '\0';
    }
#endif
    cursor++;
    if (cursor != sym_list.end()) {
        sscanf(cursor->c_str(), "%p %*c %*s\n", (void **)&end);
    }
    else {
        end = INVALID_ADDR;
    }
    sym.start = start;
    sym.end = end;
    sym.ip = start;

    syms.insert(sym);
    return true;
}

static bool load_kernel_symbol_list(std::vector<std::string> &sym_list)
{
    FILE *fp = fopen("/proc/kallsyms", "r");
    if (!fp) {
        return -1;
    }

    char buf[256];
    char type;
    int len;
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        sscanf(buf, "%*p %c %*s\n", &type);
        if ((type | 0x20) != 't') {
            continue;
        }
        len = strlen(buf);
        if (buf[len-1] == '\n') {
            buf[len-1] = ' ';
        }
        sym_list.push_back(buf);
    }
    fclose(fp);

    std::sort(sym_list.begin(), sym_list.end());
    return true;
}

bool elf_symbol::find_kernel_symbol(fsymbol &sym)
{
    load_kern();
	sym.end = sym.start = 0;
    std::set<fsymbol>::iterator it = kernel_symbols.find(sym);
    if (it != kernel_symbols.end()) {
        sym.end = it->end;
        sym.start = it->start;
        sym.name = it->name;
        return true;
    }
    return false;
}

bool elf_symbol::load_kern(void)
{
    if (kernel_symbols.size() != 0) {
        return true;
    }

	std::vector<std::string> sym_list;
	load_kernel_symbol_list(sym_list);
    std::vector<std::string>::iterator cursor = sym_list.begin();
    while (get_next_kernel_symbol(kernel_symbols, sym_list, cursor)) {
        cursor++;
    }
	std::cout<<sym_list.size()<<std::endl;
	return true;
}

elf_symbol::elf_symbol()
{
}

elf_symbol::~elf_symbol()
{

}
#if 0


  193 static bool get_next_kernel_symbol(
  194         std::set<symbol> &syms,
  195         std::vector<std::string> &sym_list,
  196         std::vector<std::string>::iterator cursor)
  197 {
  198     if (cursor == sym_list.end()) {
  199         return false;
  200     }
  201     symbol sym;
  202     size_t start, end;
  203     sscanf(cursor->c_str(), "%p %*c %*s\n", (void **)&start);
  204     sym.name = cursor->c_str() + 19;
  205     rtrim(sym.name);
  206 #if 0
  207     if (sym.name[sym.name.size()-1] == '\n') {
  208         sym.name[sym.name.size()-1] = '\0';
  209     }
  210 #endif
  211     cursor++;
  212     if (cursor != sym_list.end()) {
  213         sscanf(cursor->c_str(), "%p %*c %*s\n", (void **)&end);
  214     }
  215     else {
  216         end = INVALID_ADDR;
  217     }
  218     sym.start = start;
  219     sym.end = end;
  220     sym.ip = start;
  221
  222     syms.insert(sym);
  223     return true;
  224 }


  226 bool symbol_parser::load_kernel()
  227 {
  228     if (kernel_symbols.size() != 0) {
  229         return true;
  230     }
  231
  232     std::vector<std::string> sym_list;
  233     if (!load_kernel_symbol_list(sym_list)) {
  234         exit(0);
  235         return false;
  236     }
  237
  238     std::vector<std::string>::iterator cursor = sym_list.begin();
  239     while (get_next_kernel_symbol(kernel_symbols, sym_list, cursor)) {
  240         cursor++;
  241     }
  242     return true;
  243 }

  260 
  261 bool symbol_parser::find_kernel_symbol(symbol &sym)
  262 {
  263     load_kernel();
  264     sym.end = sym.start = 0;
  265     std::set<symbol>::iterator it = kernel_symbols.find(sym);
  266     if (it != kernel_symbols.end()) {
  267         sym.end = it->end;
  268         sym.start = it->start;
  269         sym.name = it->name;
  270         return true;
  271     }
  272     return false;
  273 }
#endif

