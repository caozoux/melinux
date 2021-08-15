#ifndef __TEMPLATE_IOCMD_H__
#define __TEMPLATE_IOCMD_H__

#ifndef u64
typedef unsigned long U64;
#endif

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
	IOCTL_USEHWPCI,
	IOCTL_USEBLOCK,
	IOCTL_USEKTIME,
	IOCTL_USESCHED,
	IOCTL_USEKRBTREE,
	IOCTL_PCI,
};

enum IOCTL_TYPE {
	IOCTL_TYPE_VMALLOC_MAX=1,
};

enum IOCTL_USEMEM_SUB{
	IOCTL_USEMEM_NONE = 0,
	IOCTL_USEMEM_PAGEDUMP,
	IOCTL_USEMEM_VMA_SCAN,
};

enum IOCTL_USEKTIME_SUB{
	IOCTL_USEKTIME_NONE = 0,
	IOCTL_USEKTIME_DEV_SCAN,
	IOCTL_USEKTIME_FILE,
};

enum IOCTL_USESCHED_SUB{
	IOCTL_USESCHED_NONE = 0,
	// get task struct 
	IOCTL_USESCHED_TASK_GET,
};

enum IOCTL_USEBLOCK_SUB{
	IOCTL_USEBLOCK_NONE = 0,
	IOCTL_USEBLOCK_INDOE,
	IOCTL_USEBLOCK_FILE,
	IOCTL_USEBLOCK_FILE_DROP_CACHE,
};

enum SLUB_OP {
	SLUB_OP_NONE = 0,
	SLUB_OP_CREATE,
	SLUB_OP_REMOVE,
	SLUB_OP_ADD,
	SLUB_OP_DEC
};

enum IOCTL_USEKMEM_SUB{
	IOCTL_USEKMEM_NONE = 0,
	//list all date of /proc/meminfo
	IOCTL_USEKMEM_SHOW,
	IOCTL_USEKMEM_GET,
	IOCTL_USEKMEM_CACHE,
	IOCTL_USEKMEM_SWAPCACHE,
	IOCTL_USEKMEM_ACTIVE_ANON_PAGE,
	IOCTL_USEKMEM_INACTIVE_ANON_PAGE,
	IOCTL_USEKMEM_ACTIVE_PAGE,
	IOCTL_USEKMEM_INACTIVE_PAGE,
	IOCTL_USEKMEM_VMA_SCAN,
	IOCTL_USEKMEM_GET_PTE, //get memory addr pte value
	IOCTL_USEKMEM_PAGE_ATTR,
	IOCTL_USEKMEM_TESTBUDDY,
	IOCTL_USEKMEM_FULL_PAGE_SCAN,
	IOCTL_USEKMEM_SLUB_OP,
	IOCTL_USEKMEM_RESOURCE_SCAN,
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
	IOCTL_HARDLOCK_IRQTRYLOCK,
	IOCTL_SEMAPHORE_DOWN,
	IOCTL_SEMAPHORE_UP,
	IOCTL_SEMAPHORE_READ_DOWN,
	IOCTL_SEMAPHORE_READ_UP,
	IOCTL_SEMAPHORE_WRITE_UP,
	IOCTL_SEMAPHORE_WRITE_DOWN
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

enum IOCTL_PCI_SUB {
	IOCTL_PCI_ENUM, //enmualte all pci device
	IOCTL_PCI_DUMPINFO, //get pci device info
	IOCTL_PCI_ENABLE, //enable pci device
	IOCTL_PCI_DISABLE, //disable pci device
};

struct mem_size_stats {
    unsigned long resident;
    unsigned long shared_clean;
    unsigned long shared_dirty;
    unsigned long private_clean;
    unsigned long private_dirty;
    unsigned long referenced;
    unsigned long anonymous;
    unsigned long lazyfree;
    unsigned long anonymous_thp;
    unsigned long shmem_thp;
    unsigned long swap;
    unsigned long shared_hugetlb;
    unsigned long private_hugetlb;
    unsigned long pss;
    unsigned long pss_locked;
    unsigned long swap_pss;
    int check_shmem_swap;
	unsigned long page_order[11];
	unsigned long *page_buffer;
	unsigned long page_index;
};

struct ioctl_data {
	enum ioctl_cmdtype type;
	int  cmdcode;
	int pid;
	char name[128];
	union {
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
			int count;
			void *buf; 
			int buf_len; 
		} radix_data;
		struct ext2fs_ioctl {
			// radixtree index
			char blk_name[128]; 
			int blk_offset;
			unsigned char *buf;
			int buf_len;
		} ext2_data;
		struct kmem_ioctl {
			char name[128];
			unsigned long extern1; //it is a extern val1
			unsigned long extern2; //it is a extern val2
			unsigned long extern3; //it is a extern val3
			unsigned long *zone_vm_state;
			unsigned long zone_len;
			unsigned long *node_vm_state;
			unsigned long node_len;
			unsigned long *numa_vm_state;
			unsigned long numa_len;
			unsigned long addr; //pte addr
			unsigned long val; //if pte, it is pte value
			struct mem_size_stats mss;
			struct zone_data {
				unsigned long       *pageblock_flags;
				unsigned long       zone_start_pfn;
				unsigned long       managed_pages;
				unsigned long       spanned_pages;
				unsigned long       present_pages;
				unsigned long       nr_isolate_pageblock;
				unsigned long       compact_cached_free_pfn;
				unsigned long       compact_cached_migrate_pfn[2];
			} zonedata;
			union {
				struct slub_control {
					enum SLUB_OP op;
					int slub_size;
					unsigned long count;
				} slub_ctrl;
				struct page_attr {
					unsigned long start_pfn;
					unsigned long size;
				}pageattr_data;
			};
		} kmem_data;

		struct sched_ioctl {
			int pid;
			struct u_task_info {
				unsigned int rq;
				unsigned long exec_start;
				unsigned long sum_exec_runtime;
				unsigned long vruntime;
				unsigned long prev_sum_exec_runtime;
			} task_info;
		} sched_data;
		struct block_ioctl {
			char filename[256];
		} block_data;
		struct pci_ioctl {
			
		} pci_data;
	};
	unsigned long args[5];
	char *log_buf;
	unsigned char *data_buf;
};


#endif
