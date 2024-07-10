#ifndef __KSCHED_H__
#define  __KSCHED_H__

enum IOCTL_KRPOBE_SUB {
	IOCTL_KSCHED_NONE = 0,
	//dumpstack of function
	IOCTL_KSCHED_DUMP,
};


struct ksched_ioctl {
	int enable;
};

#endif
