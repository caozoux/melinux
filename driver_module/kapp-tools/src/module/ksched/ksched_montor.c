#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/mm.h>

#include <ksysd.h>
#include <kpercpu.h>
#include "ksysdata.h"
#include "ksysd_ioctl.h"
#include "kbase.h"
#include "ioctl_kprobe.h"
#include "ksioctl/ksched_ioctl.h"
#include "ksched_local.h"

struct cfs_monitor_struct {
	int interval_us;
	int cpu;
	struct hrtimer timer;

} cfs_monitor_data;

static void cfs_info_dump(void)
{
	struct cfs_rq *cfs_rq = &cpu_rq_cp(2)->cfs;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(5, 14, 0)
    trace_printk("load_avg:%ld runnable_load_avg:%ld runnable_load_sum:%ld nr_ruinning:%d\n"
			, (unsigned long)cfs_rq->avg.load_avg
			, (unsigned long)cfs_rq->avg.runnable_load_avg
			, (unsigned long)cfs_rq->avg.runnable_load_sum
			, cfs_rq->nr_running
			);
#else
    trace_printk("load_avg:%ld runnable_load_avg:%ld runnable_load_sum:%ld nr_ruinning:%d\n"
			, (unsigned long)cfs_rq->avg.load_avg
			, (unsigned long)cfs_rq->avg.runnable_avg
			, (unsigned long)cfs_rq->avg.runnable_sum
			, cfs_rq->nr_running
			);
#endif
}

static void qos_info_dump(void)
{
#if LINUX_VERSION_CODE <= KERNEL_VERSION(5, 14, 0)
	struct qos_rq *qos_rq = &cpu_rq_cp(2)->qos;
    trace_printk("load_avg:%ld runnable_load_avg:%ld runnable_load_sum:%ld nr_ruinning:%d runnable_weight:%lx\n"
			, (unsigned long)qos_rq->avg.load_avg
			, (unsigned long)qos_rq->avg.runnable_load_avg
			, (unsigned long)qos_rq->avg.runnable_load_sum
			, qos_rq->qos_nr_running
			, qos_rq->runnable_weight
			);
#endif
}

static enum hrtimer_restart cfs_monitor_hrtimer(struct hrtimer *timer)
{
	struct cfs_monitor_struct *data = container_of(timer, struct cfs_monitor_struct, timer);
	ktime_t now;

 	now = ktime_get();

	hrtimer_forward(timer, now, ns_to_ktime(data->interval_us*1000));
	qos_info_dump();
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

static int monitor_pid(struct ksched_ioctl *kioctl, struct ioctl_ksdata *ksdata)
{
	struct task_struct *p = find_process_by_pid(kioctl->pid);
	struct sched_entity_patial *se = &kioctl->se;
	//struct cfs_rq_patial  *cfs_rq = &kioctl->cfs_rq;
	//struct rq_patial *rq = &kioctl->rq;

	if (!p)
		return -EINVAL;

	get_task_struct(p);
    se->weight = p->se.load.weight;
    se->load_sum = p->se.avg.load_sum;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(5, 14, 0)
    se->runnable_sum = p->se.avg.runnable_load_sum;
    se->runnable_avg = p->se.avg.runnable_load_avg;
#else
    se->runnable_sum = p->se.avg.runnable_sum;
    se->runnable_avg = p->se.avg.runnable_avg;
#endif
    se->util_sum = p->se.avg.util_sum;
    se->period_contrib = p->se.avg.period_contrib;
    se->load_avg = p->se.avg.load_avg;
    se->util_avg = p->se.avg.util_avg;

	if (copy_to_user((char __user *)ksdata->data, kioctl, sizeof(struct ksched_ioctl))) {
		pr_err("ioctl data copy err\n");
		goto fialed;
	}
	//printk("zz %s %d %ld\n", __func__, __LINE__, se->weight);
	//kioctl->cfs_rq
	//kioctl->rq
	put_task_struct(p);

	return 0;
fialed:
	return -EFAULT;
}

struct proc_dir_entry *ksched_monitor_subroot;
int ksched_monitor_ioctl_func(unsigned int cmd, unsigned long addr, struct ksched_ioctl *kioctl, struct ioctl_ksdata *ksdata)
{
	printk("zz %s cmd:%lx \n",__func__, (unsigned long)cmd);
	switch (cmd) {
		case IOCTL_KSCHED_MONITOR_PID:
			return monitor_pid(kioctl, ksdata);
		case IOCTL_KSCHED_CFS_MONITOR_TIMERR:
			if (kioctl->enable)
				cfs_monitor_timer_start(kioctl->interval_us);
			else
				cfs_monitor_timer_stop();
			break;
		default:
			break;
	}
	return 0;
}

int ksched_monitor_init(void)
{
	ksched_monitor_subroot = proc_mkdir("sched_monitor", ksys_proc_root);
	hrtimer_init(&cfs_monitor_data.timer, CLOCK_MONOTONIC, HRTIMER_MODE_ABS);

	cfs_monitor_timer_start(1000);
	return 0;
}

int ksched_monitor_exit(void)
{
	if (hrtimer_active(&cfs_monitor_data.timer))
		hrtimer_cancel(&cfs_monitor_data.timer);

	proc_remove(ksched_monitor_subroot);
	return 0;
}

