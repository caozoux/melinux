#ifndef __KSYSD_IOCTL_H__
#define __KSYSD_IOCTL_H__

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
	IOCTL_KVM,
	IOCTL_PCI,
	IOCTL_PANIC,
	IOCTL_INJECT,
	IOCTL_TRACE,
	IOCTL_INIT,
	IOCTL_KTRACE,
};

enum IOCTL_TYPE {
	IOCTL_TYPE_VMALLOC_MAX=1,
};

enum IOCTL_USEMEM_SUB{
	IOCTL_USEMEM_NONE = 0,
	IOCTL_USEMEM_PAGEDUMP,
	IOCTL_USEMEM_VMA_SCAN,
};

enum IOCTL_PANIC_SUB{
	IOCTL_PANIC_NONE = 0,
	IOCTL_PANIC_NOTIFIER,
	IOCTL_PANIC_UNNOTIFIER,
	IOCTL_PANIC_TRIGGER,
	IOCTL_PANIC_LOG,
};

enum IOCTL_USEKTIME_SUB{
	IOCTL_USEKTIME_NONE = 0,
	IOCTL_USEKTIME_DEV_SCAN,
	IOCTL_USEKTIME_HOOK_HRTIMER_TIMEOUT,
	IOCTL_USEKTIME_FILE,
};

enum IOCTL_USESCHED_SUB{
	IOCTL_USESCHED_NONE = 0,
	//get task struct 
	IOCTL_USESCHED_TASK_GET,
	IOCTL_USESCHED_CREATE_KTHREAD,
	IOCTL_USESCHED_WAKEUP_KTHREAD,
	IOCTL_USESCHED_BIND_CPU,
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
	IOCTL_USEKMEM_TESTMMAP,
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
	IOCTL_USEKRPOBE_KPROBE_FUNC, //kprobe and printk function
	IOCTL_USEKRPOBE_KPROBE_HOOK, //hook and not return real func
	IOCTL_USEKRPOBE_KPROBE_FREE, //free kprobe function
	IOCTL_USEKRPOBE_KRETPROBE_FUNC, //kretprobe  function
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

enum IOCTL_TRACE_SUB{
	IOCTL_TRACE_NONE = 0,
	IOCTL_TRACE_NULL,
	//softirq hwirq hrtimer timer
	IOCTL_TRACE_IRQ_ALL,
	IOCTL_TRACE_IRQ_HRTIMER,
	IOCTL_TRACE_IRQ_SOFTTIMER,
	IOCTL_TRACE_IRQ_HWIRQ,
	IOCTL_TRACE_IRQ_SOFTIRQ,
};

enum IOCTL_INJECT_SUB{
	IOCTL_INJECT_NONE = 0,
	IOCTL_INJECT_NULL,
	IOCTL_INJECT_WRITE_PROTECT,
	IOCTL_INJECT_MUTET_DEPLOCK,
	IOCTL_INJECT_SPINLOCK_DEPLOCK,
	IOCTL_INJECT_IRQSPINLOCK_DEPLOCK,
	IOCTL_INJECT_RUC_HANG,
	IOCTL_INJECT_SOFTWATCHDOG_TIMEOUT,
	IOCTL_INJECT_HRTIMER,
	IOCTL_INJECT_SLUB_CTRL,
	IOCTL_INJECT_SLUB_OVERWRITE,
};

enum IOCTL_KVM_SUB{
	IOCTL_KVM_NONE = 0,
	IOCTL_KVM_DUMP,
};

enum IOCTL_USEINIT_SUB{
	IOCTL_USEINIT_NONE = 0,
	IOCTL_USEINIT_INIT,
	IOCTL_USEINIT_CHECK,
};

#endif /* ifndef __KSYSD_IOCTL_H__ */

