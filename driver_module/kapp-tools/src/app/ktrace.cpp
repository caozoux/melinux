#include <iostream>
#include <string>
#include <ksioctl/ktrace_ioctl.h>

#define __FILECMD__ "ktrace"

#include "kapp_unit.h"

using namespace std;

DEFINE_string_alias(unit, "", "\n"
//DEFINE_string(unit, "", "\n"
"rcu\n"
"sched_switch\n"
);

static int test_code = 6;
//extern string FLAGS_unit;

int unit_ktrace(int argc, char** argv)
{
	struct ioctl_ksdata data;
	struct ktrace_ioctl ktrace_data;

	ktrace_data.enable = 0;
	ktrace_data.threshold = 0;

	data.data = &ktrace_data;
	data.len = sizeof(struct ktrace_ioctl);

	if (globle_unit== "rcu") {
		ktrace_data.enable = 1;
		return ktools_ioctl::kioctl(IOCTL_KTRACE, (int)IOCTL_USEKTRACE_RCU, (void*)&data, sizeof(struct ioctl_ksdata_ktrace));
	}

	if (globle_unit== "sched_switch") {
		ktrace_data.enable = 1;
		return ktools_ioctl::kioctl(IOCTL_KTRACE, (int)IOCTL_USEKTRACE_SCHED_SWITCH, (void*)&data, sizeof(struct ioctl_ksdata_ktrace));
	}

	return 0;
}

