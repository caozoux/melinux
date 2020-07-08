#ifndef __TEMPLATE_IOCMD_H__
#define __TEMPLATE_IOCMD_H__

enum ioctl_cmdtype {
	IOCTL_LOCK =1,
	IOCTL_MEM,
	IOCTL_SOFTLOCK,
	IOCTL_HARDLOCK,
	IOCTL_USERMAP,
	IOCTL_USERCU,
	IOCTL_USEKPROBE,
	IOCTL_USEWORKQUEUE,
};

enum IOCTL_TYPE {
	IOCTL_TYPE_VMALLOC_MAX=1,
};

enum IOCTL_USERCU_SUB {
	IOCTL_USERCU_NONE = 0,
	IOCTL_USERCU_READTEST_START,
	IOCTL_USERCU_READTEST_END,
};

enum IOCTL_HARDLOCK_SUB {
	IOCTL_HARDLOCK_NONE = 0,
	IOCTL_HARDLOCK_LOCK,
	IOCTL_HARDLOCK_UNLOCK,
	IOCTL_HARDLOCK_TRYLOCK,
	IOCTL_HARDLOCK_IRQLOCK,
	IOCTL_HARDLOCK_IRQUNLOCK,
	IOCTL_HARDLOCK_IRQTRYLOCK
};

enum IOCTL_USEKRPOBE_SUB {
	IOCTL_USEKRPOBE_NONE = 0,
	//dumpstack of function
	IOCTL_USEKRPOBE_FUNC_DUMP,
};

enum IOCTL_USEWORKQUEUE_SUB{
	IOCTL_USEWORKQUEUE_NONE = 0,
	//dumpstack of function
	IOCTL_USEWORKQUEUE_SIG = 0,
	IOCTL_USEWORKQUEUE_PERCPU = 0,
};

struct ioctl_data {
	enum ioctl_cmdtype type;
	union {
		int normal;
		struct kprobe_ioctl {
			char *name;
			int len;
			char *dump_buf;
			int dump_len;
		} kp_data;
		struct workqueue_ioctl {
			int runtime;
		} wq_data;
	};
	int  cmdcode;
	unsigned long args[5];
};


#endif
