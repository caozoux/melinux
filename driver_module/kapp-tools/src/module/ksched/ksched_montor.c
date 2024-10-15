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

static int monitor_pid(struct ksched_ioctl *kioctl, struct ioctl_ksdata *ksdata)
{
	struct task_struct *p = find_process_by_pid(kioctl->pid);
	struct sched_entity_patial *se = &kioctl->se;
	struct cfs_rq_patial  *cfs_rq = &kioctl->cfs_rq;
	struct rq_patial *rq = &kioctl->rq;

	if (!p)
		return -EINVAL;

	get_task_struct(p);
    se->weight = p->se.load.weight;
    se->load_sum = p->se.avg.load_sum;
    se->runnable_sum = p->se.avg.runnable_sum;
    se->util_sum = p->se.avg.util_sum;
    se->period_contrib = p->se.avg.period_contrib;
    se->load_avg = p->se.avg.load_avg;
    se->runnable_avg = p->se.avg.runnable_avg;
    se->util_avg = p->se.avg.util_avg;


	if (copy_to_user((char __user *)ksdata->data, kioctl, sizeof(struct ksched_ioctl))) {
		pr_err("ioctl data copy err\n");
		goto fialed;
	}
	printk("zz %s %d %ld\n", __func__, __LINE__, se->weight);
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
	switch (cmd) {
		case IOCTL_KSCHED_MONITOR_PID:
			return monitor_pid(kioctl, ksdata);
			break;

		default:
			
	}
	return 0;
}

int ksched_monitor_init(void)
{
	ksched_monitor_subroot = proc_mkdir("sched_monitor", ksys_proc_root);
	return 0;
}

int ksched_monitor_exit(void)
{
	return 0;
}

