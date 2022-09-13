#include <iostream>
#define __FILECMD__ "kprobe"
#include "kapp_unit.h"


DEFINE_string(func, "", "kernel function symbol name");
int unit_kprobe(int argc, char** argv)
{
  	std::cout<<"mekprbe:"<<FLAGS_func<<std::endl;
	return 0;
}
