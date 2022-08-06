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
#if LINUX_VERSION_CODE >  KERNEL_VERSION(5,0,0)
extern void flush_tlb_mm_range(struct mm_struct *mm, unsigned long start,
	unsigned long end, unsigned int stride_shift,
	bool freed_tables);
#endif
#include <linux/timekeeper_internal.h>
#include <kernel/sched/sched.h>

#include <asm/stacktrace.h>
#include "kschedlocal.h"

void dump_rt_list_busy(void)
{
	struct sched_rt_entity *rt_se = &current->rt;
	struct rt_rq *rt_rq = rt_se->rt_rq;
	printk("zz %s rt_rq:%lx \n",__func__, (unsigned long)rt_rq);
	
}
