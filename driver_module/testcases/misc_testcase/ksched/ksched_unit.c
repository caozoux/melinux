#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/notifier.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/timekeeper_internal.h>
#include <kernel/sched/sched.h>

#include <asm/stacktrace.h>
#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"
#include "medelay.h"

struct tk_core {
	seqcount_t      seq;
	struct timekeeper  timekeeper;
};

struct tk_core *orig_tk_core;

static struct task_struct *(*orig_find_process_by_pid)(pid_t pid);

struct rq *orig_runqueues;
#define cpu_rq(cpu)     (&per_cpu(*orig_runqueues, (cpu)))
int ksach_sacn_cfs_rq(void)
{
	for_each_leaf_cfs_rq(rq, cfs_rq) {
		struct sched_entity *se;
	}
}

int ksched_scan_rq(struct ioctl_data *data)
{
	struct rq *rq;
	int cpu;

	for_each_online_cpu(cpu) {
		rq = cpu_rq(cpu);
		printk("zz %s cpu:%lx nr_running:%lx %s\n",__func__, (unsigned long)cpu, (unsigned long)rq->nr_running, rq->curr->comm);
	}

#if 0
	struct rq *rq = cpu_rq(cpu);
	struct cfs_rq *cfs_rq;
	for_each_online_cpu(cpu) {
		rq = cpu_rq(cpu);
		for_each_leaf_cfs_rq(rq, cfs_rq) {

		}
	}
#else

#endif

}
int sched_unit_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_data *data)
{
	struct u_task_info  info;
	//struct  sched_iotcl *sched_io;
	struct  task_struct *task;
	switch (data->cmdcode) {
		case IOCTL_USESCHED_TASK_GET:
#if LINUX_VERSION_CODE <  KERNEL_VERSION(5,0,0)
				task = orig_find_process_by_pid(data->sched_data.pid);
#endif
				//info.exec_start;
				//info.sum_exec_runtime;
				//info.vruntime;
				//info.prev_sum_exec_runtime;
		default:
			break;
	}

	return 0;
}

int sched_unit_init(void)
{
	ktime_t now1, now2;
	struct timekeeper *tk;
	ktime_t base;
	u64 nsecs;

	LOOKUP_SYMS(runqueues);
	LOOKUP_SYMS(tk_core);
#if LINUX_VERSION_CODE <  KERNEL_VERSION(5,0,0)
	LOOKUP_SYMS(find_process_by_pid);
#endif
	tk = &orig_tk_core->timekeeper;
	base = tk->tkr_mono.base;
	nsecs = tk->tkr_mono.xtime_nsec;
	now2 = ktime_add_ns(base, nsecs);

	now1 = ktime_get();

	printk("zz %s now1:%lx now2:%lx base:%lx\n",__func__, (unsigned long)now1, (unsigned long)now2, (unsigned long)base);
	ksched_scan_rq(NULL);

	return 0;
}

int sched_unit_exit(void)
{
	return 0;
}

