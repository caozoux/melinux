#include <iostream>
#define __FILECMD__ "kinject"
#include "kapp_unit.h"

DEFINE_bool(hrtime, false, "inject hrtime");
DEFINE_int32(delay, -1, "delay useconds time");
int unit_kinject(int argc, char** argv)
{
  	std::cout<<"metest:"<<FLAGS_hrtime	<<std::endl;
	return 0;
}
