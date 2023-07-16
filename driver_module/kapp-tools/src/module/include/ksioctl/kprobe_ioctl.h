#ifndef __KBLOCK_H__
#define  __KBLOCK_H__


enum IOCTL_KRPOBE_SUB {
	IOCTL_KRPOBE_NONE = 0,
	//dumpstack of function
	IOCTL_KRPOBE_FUNC_DUMP,
	IOCTL_KRPOBE_FUNC_DUMP_CLEAN,
	//delay specfiy function with delay
	IOCTL_KPROBE_SUB_DELAY,
	//show all kprobe register
	IOCTL_KPROBE_SUB_LIST,
};


struct kprobe_ioctl {
	int enable;
	unsigned long delay;
	char name[256];
};

#endif

