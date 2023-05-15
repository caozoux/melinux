#ifndef __KTRACE_IOCTL_H__
#define __KTRACE_IOCTL_H__

struct kstack_ioctl {
	void *buf;
	int size;
};

enum IOCTL_KSTACK_SUB{
	IOCTL_KSTACK_NONE = 0,
	IOCTL_KSTACK_DUMP,
	IOCTL_KSTACK_CLEAN,
};

#define MAX_DEPTACH 127
#define MAX_SAVE    128

typedef struct {
	int count;
	unsigned long key;
	int offset;
	unsigned long address[MAX_DEPTACH];

} ksys_callchain;


#endif /* ifndef __KTRACE_IOCTL_H__
 */
