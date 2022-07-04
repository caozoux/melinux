#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/of.h>
#include <linux/kthread.h>
#include <linux/reboot.h>
#include <linux/uaccess.h>
#include <linux/notifier.h>
#include <linux/interrupt.h>
#include <linux/rcupdate.h>
#include <linux/delay.h>
#include <linux/clockchips.h>

#include <linux/hrtimer.h>
#include <asm/stacktrace.h>
#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"
#include "medelay.h"
#include "mekernel.h"
#include "ktimelocal.h"
#include "kernel/time/tick-sched.h"

struct tick_sched *orig_tick_cpu_sched;
struct hrtimer_cpu_base *orig_hrtimer_bases;
struct tick_sched *cpu0_tick_sched;

static void (*orig_tick_nohz_restart_sched_tick)(struct tick_sched *ts, ktime_t now);

static void smp_call_func_test(void *type)
{
	ktime_t now = ktime_get();
	printk("zz %s %d \n", __func__, __LINE__);
	orig_tick_nohz_restart_sched_tick(cpu0_tick_sched, now);
}

static void tick_renable(void)
{
	cpu0_tick_sched = &per_cpu(*orig_tick_cpu_sched, 0);
	if (!hrtimer_active(&cpu0_tick_sched->sched_timer)) {
		printk("cpu0 tick sched not active, try to enable\n");
		smp_call_function_single(0, smp_call_func_test, NULL, 1);
	} else {
		printk("cpu0 tick sched is active, ignore\n");
	}


	//hrtimer_forward(timer, now, tick_period);

	//hrtimer_forward(timer, now, tick_period);
}

int tick_sched_init(void)
{
	LOOKUP_SYMS(tick_cpu_sched);
	LOOKUP_SYMS(tick_nohz_restart_sched_tick);
	LOOKUP_SYMS(hrtimer_bases);
	tick_renable();
	//printk("zz %s orig_tick_cpu_sched:%lx \n",__func__, (unsigned long)orig_tick_cpu_sched);
	//printk("zz %s cpu0_tick_cpu_sched:%lx \n",__func__, (unsigned long)cpu0_tick_cpu_sched);

	return 0;
}

void tick_sched_exit(void)
{

}

