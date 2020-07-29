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
	IOCTL_USERAIDIXTREE,
	IOCTL_USEREXT2,
};

enum IOCTL_TYPE {
	IOCTL_TYPE_VMALLOC_MAX=1,
};

enum IOCTL_USEEXT2_SUB{
	IOCTL_USEEXT2_NONE = 0,
	//遍历supper_block
	IOCTL_USEEXT2_ENUM_SUPBLOCK,
	IOCTL_USEEXT2_GET_BLOCK,
};

enum IOCTL_USERAIDIXTREE_SUB {
	IOCTL_USERAIDIXTREE_NONE = 0,
	IOCTL_USERAIDIXTREE_ADD,
	IOCTL_USERAIDIXTREE_DEL,
	IOCTL_USERAIDIXTREE_GET,
	IOCTL_USERAIDIXTREE_DUMP,
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
	IOCTL_USEWORKQUEUE_SIG,
	IOCTL_USEWORKQUEUE_SIG_SPINLOCK,
	IOCTL_USEWORKQUEUE_SIG_SPINLOCKIRQ,
	IOCTL_USEWORKQUEUE_PERCPU,
	IOCTL_USEWORKQUEUE_PERCPU_SPINLOCKIRQ_RACE,
	IOCTL_USEWORKQUEUE_PEFORMANCE_DELAY,
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
			unsigned long *workqueue_performance;
		} wq_data;
		struct raidixtree_ioctl {
			// radixtree index
			int index; 
			void *buf; 
			int buf_len; 
		} raidix_data;
		struct ext2fs_ioctl {
			// radixtree index
			char blk_name[128]; 
			int blk_offset;
			unsigned char *buf;
			int buf_len;
		} ext2_data;
	};
	int  cmdcode;
	unsigned long args[5];
};


#endif
