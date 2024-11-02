#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */
#include <linux/kallsyms.h>
#include <linux/kprobes.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include "kernel/sched/sched.h"

#define LOOKUP_SYMS(name) do {							\
		orig_##name = (void *)cust_kallsyms_lookup_name(#name);		\
		if (!orig_##name) {						\
			pr_err("kallsyms_lookup_name: %s\n", #name);		\
			return -EINVAL;						\
		}								\
	} while (0)

static char ksym_name[KSYM_NAME_LEN] = "pid_max";
module_param_string(ksym, ksym_name, KSYM_NAME_LEN, S_IRUGO);
MODULE_PARM_DESC(ksym, "Kernel symbol to monitor; this module will report any"
			" write operations on the kernel symbol");

unsigned long (*cust_kallsyms_lookup_name)(const char *name);
static int noop_pre_handler(struct kprobe *p, struct pt_regs *regs) { return 0; }

struct rq __percpu * orig_runqueues;
#define this_rq_cp() this_cpu_ptr(orig_runqueues)
#define cpu_rq_cp(cpu)  (&per_cpu(*orig_runqueues, (cpu)))

static struct proc_dir_entry *proc_root;

struct cfs_monitor_struct {
	int interval_us;
	int cpu;
	struct hrtimer timer;

} cfs_monitor_data;

static void cfs_info_dump(void)
{
	struct cfs_rq *cfs_rq = &cpu_rq_cp(3)->cfs;
    trace_printk("load_avg:%ld load_sum:%ld runnable_load_avg:%ld runnable_load_sum:%ld nr_ruinning:%d\n"
			, (unsigned long)cfs_rq->avg.load_avg
			, (unsigned long)cfs_rq->avg.load_sum
			, (unsigned long)cfs_rq->avg.runnable_load_avg
			, (unsigned long)cfs_rq->avg.runnable_load_sum
			, cfs_rq->nr_running
			);
}

static void qos_info_dump(void)
{
	struct qos_rq *qos_rq = &cpu_rq_cp(3)->qos;
    trace_printk("load_avg:%ld load_sum:%ld runnable_load_avg:%ld runnable_load_sum:%ld nr_ruinning:%d runnable_weight:%lx\n"
			, (unsigned long)qos_rq->avg.load_avg
			, (unsigned long)qos_rq->avg.load_sum
			, (unsigned long)qos_rq->avg.runnable_load_avg
			, (unsigned long)qos_rq->avg.runnable_load_sum
			, qos_rq->qos_nr_running
			, qos_rq->runnable_weight
			);
}

static enum hrtimer_restart cfs_monitor_hrtimer(struct hrtimer *timer)
{
	struct cfs_monitor_struct *data = container_of(timer, struct cfs_monitor_struct, timer);
	ktime_t now;

 	now = ktime_get();

	hrtimer_forward(timer, now, ns_to_ktime(data->interval_us*1000));
	//qos_info_dump();
	cfs_info_dump();
	return HRTIMER_RESTART;
}

static void cfs_monitor_timer_start(int timeout)
{
	ktime_t kt;

	cfs_monitor_data.interval_us = timeout;
	cfs_monitor_data.timer.function = cfs_monitor_hrtimer;
	kt = ktime_add_us(ktime_get(), timeout);

	hrtimer_set_expires(&cfs_monitor_data.timer, kt);
	hrtimer_start_expires(&cfs_monitor_data.timer, HRTIMER_MODE_ABS_PINNED);
}

static void cfs_monitor_timer_stop(void)
{
	if (hrtimer_active(&cfs_monitor_data.timer))
		hrtimer_cancel(&cfs_monitor_data.timer);
	else
		printk("WARN: cfs monitor timer is disable\n");
}

static int get_kallsyms_lookup_name(void)
{
	int ret;
	struct kprobe kp;

	memset(&kp, 0, sizeof(struct kprobe));

	ret = -1;
	kp.symbol_name = "kallsyms_lookup_name";
	kp.pre_handler = noop_pre_handler;

	ret = register_kprobe(&kp);
	if (ret < 0) {
		printk("Err: find kallsyms_lookup_name failed \n");
		return -EINVAL;
	}

	cust_kallsyms_lookup_name = (void*)kp.addr;
	unregister_kprobe(&kp);

	return 0;
}

static int sym_init(void)
{
	if (get_kallsyms_lookup_name())
		return -EINVAL;

	LOOKUP_SYMS(runqueues);

	return 0;
}

static int __init hw_break_module_init(void)
{
	if (sym_init())
		return -EINVAL;

	proc_mkdir("sched_monitor", proc_root);
	hrtimer_init(&cfs_monitor_data.timer, CLOCK_MONOTONIC, HRTIMER_MODE_ABS);

	cfs_monitor_timer_start(100);

	return 0;
}

static void __exit hw_break_module_exit(void)
{
	if (hrtimer_active(&cfs_monitor_data.timer))
		hrtimer_cancel(&cfs_monitor_data.timer);

	remove_proc_entry("sched_monitor", NULL);
}

module_init(hw_break_module_init);
module_exit(hw_break_module_exit);

MODULE_LICENSE("GPL");

