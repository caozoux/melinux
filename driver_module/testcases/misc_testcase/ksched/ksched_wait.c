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
#include <linux/swait.h>
#include <kernel/sched/sched.h>

#include <asm/stacktrace.h>
#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "kschedlocal.h"
#include "debug_ctrl.h"
#include "medelay.h"

static struct swait_queue_head swait_wq;
//static int swait_wq_contd = 0;
static struct task_struct *swait_timeout_kthread =NULL;
//static int debug11 = 0;

static bool swait_check_wake(int *gfp)
{

	printk("zz %s %d \n", __func__, __LINE__);
	return true;
	//swake_up_one(&swait_wq);
	//return false;
}

int swait_kthread_run(void *data)
{
	u64 ret ;
	int gf = 0;

	while (!kthread_should_stop()) {
		ret = swait_event_idle_timeout_exclusive(swait_wq,
				swait_check_wake(&gf), HZ * 5);
		//printk("zz %s ret:%lx %lx\n",__func__, (unsigned long)ret, jiffies);
		//if (debug11++ > 4) {
		//	debug11 = 0;
		//	break;
		//}
	}
	return 0;
}

void swait_uint_ioctl_wakeup(void *data)
{

}

void swait_uint_int(void *data)
{
	init_swait_queue_head(&swait_wq);
	swait_timeout_kthread = kthread_create(swait_kthread_run, (void *)NULL, "swait_kthread_run");
	wake_up_process(swait_timeout_kthread);
}

void swait_uint_exit(void *data)
{
	if (swait_timeout_kthread)
		kthread_stop(swait_timeout_kthread);
}

