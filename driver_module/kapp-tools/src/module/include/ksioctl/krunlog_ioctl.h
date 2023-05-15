#ifndef __KTRACE_IOCTL_H__
#define __KTRACE_IOCTL_H__

enum IOCTL_RUNLOG_SUB{
	IOCTL_RUNLOG_NONE = 0,
	IOCTL_RUNLOG_DUMP,
	IOCTL_RUNLOG_CLEAN,
};

struct krunlog_ioctl {
	void *buf;
	int len;
};

#endif /* ifndef __KTRACE_IOCTL_H__ */

