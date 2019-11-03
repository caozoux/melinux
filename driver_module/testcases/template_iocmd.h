#ifndef __TEMPLATE_IOCMD_H__
#define __TEMPLATE_IOCMD_H__

enum IOCTL_CMDTYPE {
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
struct ioctl_data {
	int  cmdtype;
	int  cmdcode;
	unsigned long args[5];
};


#endif
