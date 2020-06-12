#ifndef __TEMPLATE_IOCMD_H__
#define __TEMPLATE_IOCMD_H__

enum ioctl_cmdtype {
	IOCTL_LOCK =1,
	IOCTL_MEM,
	IOCTL_SOFTLOCK,
	IOCTL_HARDLOCK,
	IOCTL_USERMAP,
	IOCTL_USERCU,
};

enum IOCTL_TYPE {
	IOCTL_TYPE_VMALLOC_MAX=1,
};

enum IOCTL_USERCU_SUB {
	IOCTL_USERCU_NONE = 0,
	IOCTL_USERCU_READTEST_START,
	IOCTL_USERCU_READTEST_END,
};

struct ioctl_data {
	enum ioctl_cmdtype type;
	union {
		int normal;
	};
	int  cmdcode;
	unsigned long args[5];
};


#endif
