#ifndef _KNET_IOCLT_H_
#define _KNET_IOCLT_H_

enum IOCTL_USEKNET_SUB{
	IOCTL_USEKNET_NONE = 0,
};

struct knet_ioctl {
	int enable;
	int subtype;
	int trace_bits;
};

#endif
