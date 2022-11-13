#include <iostream>
#include <string>
#include <ksioctl/ktrace_ioctl.h>

#define __FILECMD__ "ktrace"

#include "kapp_unit.h"

using namespace std;

DEFINE_string(utaskunit, "", "\n"
"rcu\n"
"sched_switch\n"
);
DEFINE_int32(thread, 1, "thread");

static int test_code = 6;
//extern string FLAGS_unit;

int unit_utask(int argc, char** argv)
{
	return 0;
}

