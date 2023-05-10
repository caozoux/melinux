#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/kprobes.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/smpboot.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/nmi.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/tracepoint.h>
#include <linux/seq_file.h>
#include <linux/blkdev.h>
#include <linux/proc_fs.h>
#include <linux/oom.h>

#include "fault_event.h"

#define LOOKUP_SYMS(name) do {							\
		orig_##name = (void *)kallsyms_lookup_name(#name);		\
		if (!orig_##name) {						\
			pr_err("kallsyms_lookup_name: %s\n", #name);		\
			return -EINVAL;						\
		}								\
	} while (0)

void mkprobe_ftrace_handler(unsigned long ip, unsigned long parent_ip,
		 struct ftrace_ops *ops, struct pt_regs *regs);

struct fault_event_item {
	char *func_name;
	u64  address;
	void *ftrace_func;
};

enum fault_event_type {
	EV_DO_CORDUMP = 0,
	EV_OUT_OF_MEMORY,
	EV_OOM_KILL,
	EV_WARN_ALLOC,
	EV_WARN_PRINT,
	EV_RCU_STALL,
	EV_SOFTLOCKUP,
};

static int zero;
static int one = 1;
unsigned int sysctl_fault_event_enable = 1;
unsigned int sysctl_fault_event_print;
//unsigned int sysctl_panic_on_fatal_event;
static atomic_t tot_fault_cnt;
static atomic_t class_fault_cnt[FAULT_CLASSS_MAX];

static char *fault_class_name[FAULT_CLASSS_MAX] = {
    "Slight",
    "Normal",
    "Fatal"
};

struct proc_dir_entry *fault_event_proc;

static int kretprobe_machine_check_poll(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	int ret = regs->ax;
	printk("zz %s ret:%lx \n",__func__, (unsigned long)ret);
	return 0;
}

struct kretprobe kretprobe = {
	.kp.symbol_name = "machine_check_poll",
	.handler= kretprobe_machine_check_poll,
};

struct fault_event_item fault_event_hook_list[]  = {
	{"do_coredump", 0, &mkprobe_ftrace_handler},
	{"oom_kill_process", 0, &mkprobe_ftrace_handler},
	{"__oom_kill_process", 0, &mkprobe_ftrace_handler},
	{"warn_alloc", 0, &mkprobe_ftrace_handler},
	{"__warn", 0, &mkprobe_ftrace_handler},
	{"rcu_check_gp_kthread_starvation", 0, &mkprobe_ftrace_handler},
	{"watchdog_timer_fn", 0, &mkprobe_ftrace_handler},
	//{"print_req_error", 0, &mkprobe_ftrace_handler},
	//{"report_softlockup", 0, &mkprobe_ftrace_handler},
};

struct fault_event_item fault_event_ext4_hook_list[]  = {
	{"__ext4_error", 0, &mkprobe_ftrace_handler},
	{"__ext4_error_inode", 0, &mkprobe_ftrace_handler},
	{"__ext4_error_file", 0, &mkprobe_ftrace_handler},
	{"__ext4_std_error", 0, &mkprobe_ftrace_handler},
	{"__ext4_abort", 0, &mkprobe_ftrace_handler},
	{"__ext4_warning", 0, &mkprobe_ftrace_handler},
	{"__ext4_warning_inode", 0, &mkprobe_ftrace_handler},
	{"__ext4_grp_locked_error", 0, &mkprobe_ftrace_handler},
};


int (*orig_cgroup_mkdir)(struct kernfs_node *parent_kn, const char *name, umode_t mode);
struct tracepoint *orig___tracepoint_sched_process_hang;
struct tracepoint *orig___tracepoint_block_rq_complete;
bool (*orig_kvm_check_and_clear_guest_paused)(void);
unsigned long *orig_watchdog_touch_ts;
int (*orig_watchdog_thresh);
u64 (*orig_running_clock)(void);

int sym_init(void)
{
	int i;

	LOOKUP_SYMS(watchdog_thresh);
	LOOKUP_SYMS(running_clock);
	LOOKUP_SYMS(kvm_check_and_clear_guest_paused);
	LOOKUP_SYMS(watchdog_touch_ts);
	LOOKUP_SYMS(cgroup_mkdir);
	LOOKUP_SYMS(__tracepoint_sched_process_hang);
	LOOKUP_SYMS(__tracepoint_block_rq_complete);

	for (i = 0; i < ARRAY_SIZE(fault_event_hook_list); ++i) {
		u64 addr;
		addr = (u64)kallsyms_lookup_name(fault_event_hook_list[i].func_name);
		if (!addr) {
			printk("Err: not find %s\n", fault_event_hook_list[i].func_name);
			goto out;
		}
		fault_event_hook_list[i].address = addr;
	}

	return 0;
out:
	return -EINVAL;
}

static void trace_sched_process_hang(void *ignore, struct task_struct *tsk)
{
	report_fault_event(-1, tsk, NORMAL_FAULT, FE_HUNGTASK, NULL);
}

static void trace_block_rq_complete(void *ignore, struct request *req, int error, unsigned int nr_bytes)
{
	if (!req->bio)
		return;
	if (unlikely(error && !blk_rq_is_passthrough(req) &&
			!(req->rq_flags & RQF_QUIET)))
		report_fault_event(smp_processor_id(), current,
				FATAL_FAULT, FE_IO_ERR, NULL);

}

static int get_softlockup_thresh(void)
{
	return *orig_watchdog_thresh * 2;
}

static unsigned long get_timestamp(void)
{
	return orig_running_clock() >> 30LL;  /* 2^30 ~= 10^9 */
}

void handle_softlockup_event(void)
{
	unsigned long touch_ts = __this_cpu_read(*orig_watchdog_touch_ts);
	unsigned long now;

	if (touch_ts == 0)
		return;

	now = get_timestamp();
	if (time_after(now, touch_ts + get_softlockup_thresh())) {
		if (orig_kvm_check_and_clear_guest_paused())
			return;
		printk("zz %s %d \n", __func__, __LINE__);
		report_fault_event(smp_processor_id(), current,FATAL_FAULT, FE_SOFTLOCKUP, NULL);
	}

}

static inline bool is_memcg_oom(struct oom_control *oc)
{	
    return oc->memcg != NULL;
}

void handle_oom_event(struct pt_regs *regs)
{
	struct oom_control *oc = (struct oom_control *)regs->di;
	if (oc) {
		report_fault_event(smp_processor_id(), current, NORMAL_FAULT,
			is_memcg_oom(oc) ? FE_OOM_CGROUP : FE_OOM_GLOBAL, NULL);
	}
}

void mkprobe_ftrace_handler(unsigned long ip, unsigned long parent_ip,
		 struct ftrace_ops *ops, struct pt_regs *regs)
{
	int i;
	enum fault_event_type fevent_type = -1;

	for (i = 0; i < ARRAY_SIZE(fault_event_hook_list); i++) {
		if (fault_event_hook_list[i].address == ip) {
			fevent_type = i;
			break;
		}
	}

	switch (i) {
		case EV_DO_CORDUMP:
			report_fault_event(smp_processor_id(), current, SLIGHT_FAULT, FE_SIGNAL, NULL);
			break;
		case EV_OUT_OF_MEMORY:
			handle_oom_event(regs);
			break;
		case EV_OOM_KILL:
			report_fault_event(smp_processor_id(), current, FATAL_FAULT, FE_SIGNAL, NULL);
			break;
		case EV_WARN_ALLOC:
			report_fault_event(smp_processor_id(), current, FATAL_FAULT, FE_ALLOCFAIL, NULL);
			break;
		case EV_WARN_PRINT:
			report_fault_event(smp_processor_id(), current, FATAL_FAULT, FE_WARN, NULL);
			break;
		case EV_RCU_STALL:
			report_fault_event(smp_processor_id(), current,FATAL_FAULT, FE_RCUSTALL, NULL);
			break;
		case EV_SOFTLOCKUP:
			handle_softlockup_event();
			break;
		default:			
			printk_ratelimited("type:%d is invalid\n", i);
			break;
	}
}

static struct ftrace_ops kprobe_ftrace_ops __read_mostly = {
    .func = mkprobe_ftrace_handler,
    .flags = FTRACE_OPS_FL_SAVE_REGS | FTRACE_OPS_FL_IPMODIFY,
};

static int hrtimer_pr_init(void)
{
	int ret;
	int i;

	ret = register_kretprobe(&kretprobe);
	if (ret) {
		printk("kprobe register failed\n");
		goto out3;
	}

	ret = tracepoint_probe_register(orig___tracepoint_sched_process_hang, trace_sched_process_hang, NULL);
	if (ret)
		goto out2;

	ret = tracepoint_probe_register(orig___tracepoint_block_rq_complete, trace_block_rq_complete, NULL);
	if (ret)
		goto out1;

	for (i = 0; i < ARRAY_SIZE(fault_event_hook_list); i++) {
		ret = ftrace_set_filter_ip(&kprobe_ftrace_ops,(unsigned long)fault_event_hook_list[i].address, 0, 0);
		if (ret) {
			printk("Err: ftrace filter ip failed:%s\n", fault_event_hook_list[i].func_name);
			goto out;
		}
	}

	ret = register_ftrace_function(&kprobe_ftrace_ops);
	if (ret) {
		printk("Err: ftrace function failed:%s\n", fault_event_hook_list[i].func_name);
		goto out;
	}
	return 0;

out:
	for (i = i-1 ;i>=0;i--) {
		ftrace_set_filter_ip(&kprobe_ftrace_ops,(unsigned long)fault_event_hook_list[i].address, 1, 0);
	}

	tracepoint_probe_unregister(orig___tracepoint_block_rq_complete, trace_block_rq_complete, NULL);
out1:
	tracepoint_probe_unregister(orig___tracepoint_sched_process_hang, trace_sched_process_hang, NULL);
out2:
	unregister_kretprobe(&kretprobe);
out3:
	return -EINVAL;
}

static struct fault_event fevents[FE_MAX] = {
    {FE_SOFTLOCKUP, "soft lockup", "general", {0} },
    {FE_RCUSTALL, "rcu stall", "general", {0} },
    {FE_HUNGTASK, "hung task", "general", {0} },
    {FE_OOM_GLOBAL, "global oom", "mem", {0} },
    {FE_OOM_CGROUP, "cgroup oom", "mem", {0} },
    {FE_ALLOCFAIL, "alloc failed", "mem", {0} },
    //{FE_LIST_CORRUPT, "list corruption", "general", {0} },
    //{FE_MM_STATE, "bad mm_struct", "mem", {0} },
    {FE_IO_ERR, "io error", "io", {0} },
    {FE_EXT4_ERR, "ext4 fs error", "fs", {0} },
    {FE_MCE, "mce", "hardware", {0} },
    {FE_SIGNAL, "fatal signal", "general", {0} },
    {FE_WARN, "warning", "general", {0} },
    //{FE_PANIC, "panic", "general", {0} },
};

bool fault_monitor_enable(void)
{
    return sysctl_fault_event_enable;
}

static const char *get_task_cmdline(struct task_struct *tsk, char *buff,
        int size)
{
    struct mm_struct *mm;
    char *p = buff, c;
    int i, len, count = 0;

    if (!tsk)
        return "nil";

    if (tsk->tgid != current->tgid || !tsk->mm
        || (tsk->flags & PF_KTHREAD))
        goto use_comm;

    mm = tsk->mm;
    len = mm->arg_end - mm->arg_start;
    len = min(len, size);
    if (len <= 0)
        goto use_comm;

    if (__copy_from_user_inatomic(p, (void *)mm->arg_start, len))
        goto use_comm;

    if (__copy_from_user_inatomic(&c, (void *)(mm->arg_end - 1), 1))
        goto use_comm;

    count += len;
    if (c == '\0' || len == size)
        goto out;

    p = buff + len;
    len = mm->env_end - mm->env_start;
    len = min(len, size - count);
    if (len <= 0)
        goto out;

    if (!__copy_from_user_inatomic(p, (void *)mm->env_start, len))
        count += len;

out:
    for (i = 0; i < count-1; i++) {
        if (buff[i] == '\0')
            buff[i] = ' ';
    }
    buff[count - 1] = '\0';

    return buff;

use_comm:
    return tsk->comm;
}

void report_fault_event(int cpu, struct task_struct *tsk,
        enum FAULT_CLASS class, enum FAULT_EVENT event,
        const char *msg)
{   
    unsigned int evt_cnt;
    char tsk_cmdline[256];
    
    if (!sysctl_fault_event_enable)
        return;
    
    if (class >= FAULT_CLASSS_MAX || event >= FE_MAX)
        return;
    
    evt_cnt = atomic_inc_return(&fevents[event].count);
    atomic_inc(&class_fault_cnt[class]);
    atomic_inc(&tot_fault_cnt);
    
    if (!sysctl_fault_event_print)
        goto may_panic;
    
    printk_ratelimited(KERN_EMERG "%s fault event[%s:%s]: %s. "
        "At cpu %d task %d(%s). Total: %d\n",
        fault_class_name[class], fevents[event].module,
        fevents[event].name, msg ? msg : "", cpu,
        tsk ? tsk->pid : -1,
        //"NULL", evt_cnt);
        get_task_cmdline(tsk, tsk_cmdline, 256), evt_cnt);

may_panic:
#if 0
    if (sysctl_panic_on_fatal_event && class == FATAL_FAULT &&
        event != FE_PANIC) {
        sysctl_fault_event_enable = false;
        panic("kernel fault event");
    }
#endif
	return;
}
EXPORT_SYMBOL(report_fault_event);

static int fault_events_show(struct seq_file *m, void *v)
{
    unsigned int evt_cnt, class_cnt, total;
    int i;

	total = 0;
    total = atomic_read(&tot_fault_cnt);
    seq_printf(m, "\nTotal fault events: %d\n\n", total);

    for (i = 0; i < FAULT_CLASSS_MAX; i++) {
        class_cnt = atomic_read(&class_fault_cnt[i]);
        seq_printf(m, "%s: %d\n", fault_class_name[i],
        class_cnt);
    }

    seq_puts(m, "\n");
    for (i = 0; i < FE_MAX; i++) {
        evt_cnt = atomic_read(&fevents[i].count);
        seq_printf(m, "%s: %d\n", fevents[i].name,
            evt_cnt);
    }

    return 0;
}

static int fault_events_open(struct inode *inode, struct file *filp)
{
    return single_open(filp, fault_events_show, NULL);
}

const struct file_operations fault_events_fops = {
    .open = fault_events_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

static int fault_events_init(void)
{
    fault_event_proc = proc_create("fault_events", 0644, NULL, &fault_events_fops);
	if (!fault_event_proc)
		return -EINVAL;

    return 0;
}

static void fault_events_exit(void)
{
	proc_remove(fault_event_proc);
}

static void hrtimer_pr_exit(void)
{
	int ret;
	int i;
	ret = unregister_ftrace_function(&kprobe_ftrace_ops);
	if (ret<0) {
		printk("Err: unregister fault event ftrace failed\n");
		return;
	}

	for (i = 0; i < ARRAY_SIZE(fault_event_hook_list); i++) {
		ret = ftrace_set_filter_ip(&kprobe_ftrace_ops,(unsigned long)fault_event_hook_list[i].address, 1, 0);
		if (ret<0)
			printk("Err: disable fault event %s failed\n\n", fault_event_hook_list[i].func_name);
	}

	tracepoint_probe_unregister(orig___tracepoint_sched_process_hang, trace_sched_process_hang, NULL);
	tracepoint_probe_unregister(orig___tracepoint_block_rq_complete, trace_block_rq_complete, NULL);
	unregister_kretprobe(&kretprobe);
}

 struct ctl_table_header *fault_event_sysctrl;
static struct ctl_path kern_path[] = { { .procname = "kernel", }, { } };
static struct ctl_table pid_ns_ctl_table[] = {
    {
        .procname = "fault_event_enable",
		.data     = &sysctl_fault_event_enable,
        .maxlen = sizeof(unsigned int),
        .mode = 0644,
        .proc_handler = proc_dointvec_minmax,
        .extra1 = &zero,
        .extra2 = &one,
    },
    {
        .procname = "sysctl_fault_event_print",
		.data     = &sysctl_fault_event_print,
        .maxlen = sizeof(unsigned int),
        .mode = 0644,
        .proc_handler = proc_dointvec_minmax,
        .extra1 = &zero,
        .extra2 = &one,
    },
    { }
};

static int __init percpu_hrtimer_init(void)
{

  if (sym_init())
    return -EINVAL;

  fault_event_sysctrl = register_sysctl_paths(kern_path, pid_ns_ctl_table);
  if (!fault_event_sysctrl) {
	printk("Err: register sysctrl proc failed \n:");
	return -EINVAL;
  }

  if (fault_events_init()) {
	printk("Err: fault event proc register failed\n:");
	return -EINVAL;
  }

  hrtimer_pr_init();	
  return 0;
}

static void __exit percpu_hrtimer_exit(void)
{
	fault_events_exit();
	hrtimer_pr_exit();
	unregister_sysctl_table(fault_event_sysctrl);
}

module_init(percpu_hrtimer_init);
module_exit(percpu_hrtimer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
