#include <iostream>
#include <string>

#include "kapp_unit.h"

int unit_krunlog(int argc, char** argv)
{
	struct ioctl_ksdata data;
	struct krunlog_ioctl krunlog_data;

	krunlog_data.enable = 0;
	krunlog_data.threshold = 0;

	data.data = &krunlog_data;
	data.len = sizeof(struct krunlog_ioctl);

	if (globle_unit== "rcu") {
		krunlog_data.enable = 1;
		return ktools_ioctl::kioctl(IOCTL_KTRACE, (int)IOCTL_USEKTRACE_RCU, (void*)&data, sizeof(struct ioctl_ksdata_krunlog));
	}

	if (globle_unit== "sched_switch") {
		krunlog_data.enable = 1;
		return ktools_ioctl::kioctl(IOCTL_KTRACE, (int)IOCTL_USEKTRACE_SCHED_SWITCH, (void*)&data, sizeof(struct ioctl_ksdata_krunlog));
	}

	return 0;
}

