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
	IOCTL_USEEXT2,
	IOCTL_USEATOMIC,
	IOCTL_USEMEM,
	IOCTL_USEDEVBUSDRV,
	IOCTL_USEKMEM,
	IOCTL_USEMSR,
};

enum IOCTL_TYPE {
	IOCTL_TYPE_VMALLOC_MAX=1,
};

/*
MemTotal:       20555040 kB
MemFree:        19906432 kB
MemAvailable:   19961804 kB
Buffers:            2076 kB
Cached:           325324 kB
SwapCached:            0 kB
Active:           195380 kB
Inactive:         281648 kB
Active(anon):     144536 kB
Inactive(anon):     8484 kB
Active(file):      50844 kB
Inactive(file):   273164 kB
Unevictable:           0 kB
Mlocked:               0 kB
SwapTotal:             0 kB
SwapFree:              0 kB
Dirty:                 0 kB
Writeback:             0 kB
AnonPages:        149656 kB
Mapped:           103416 kB
Shmem:              8680 kB
Slab:              56960 kB
SReclaimable:      23708 kB
SUnreclaim:        33252 kB
KernelStack:        3712 kB
PageTables:         4972 kB
NFS_Unstable:          0 kB
Bounce:                0 kB
WritebackTmp:          0 kB
CommitLimit:    10277520 kB
Committed_AS:     824584 kB
VmallocTotal:   34359738367 kB
VmallocUsed:           0 kB
VmallocChunk:          0 kB
Percpu:             5328 kB
HardwareCorrupted:     0 kB
AnonHugePages:     40960 kB
ShmemHugePages:        0 kB
ShmemPmdMapped:        0 kB
CmaTotal:          65536 kB
*/

enum IOCTL_USEMEM_SUB{
	IOCTL_USEMEM_NONE = 0,
	//list all date of /proc/meminfo
	IOCTL_USEMEM_SHOW,
	IOCTL_USEMEM_CACHE,
	IOCTL_USEMEM_SWAPCACHE,
	IOCTL_USEMEM_ACTIVE_ANON_PAGE,
	IOCTL_USEMEM_INACTIVE_ANON_PAGE,
	IOCTL_USEMEM_ACTIVE_PAGE,
	IOCTL_USEMEM_INACTIVE_PAGE,
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

enum IOCTL_USEATOMIC_SUB {
	IOCTL_USEATOMIC_NONE = 0,
	IOCTL_USEATOMIC_PERFORMANCE,
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
