#ifndef __KTRACE_IOCTL_H__
#define __KTRACE_IOCTL_H__

struct kdevice_ioctl {
};

enum IOCTL_USEKDEVICE_SUB{
	IOCTL_USEKDEVICE_NONE = 0,
	//scan all net device
	IOCTL_USEKDEVICE_SCAN_NET,
};

#endif /* ifndef __KTRACE_IOCTL_H__
 */
