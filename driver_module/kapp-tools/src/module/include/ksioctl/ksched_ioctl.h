#ifndef __KTRACE_IOCTL_H__
#define __KTRACE_IOCTL_H__

struct ktrace_ioctl {
	int enable;
	int threshold;
};

enum IOCTL_USEKTRACE_SUB{
	IOCTL_USEKTRACE_NONE = 0,
	IOCTL_USEKTRACE_RCU,
	IOCTL_USEKTRACE_SCHED_SWITCH,
	IOCTL_USEKTRACE_NR
};

#endif /* ifndef __KTRACE_IOCTL_H__
 */
