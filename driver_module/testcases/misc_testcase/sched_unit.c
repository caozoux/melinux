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
#include <linux/delay.h>
#include <linux/timekeeper_internal.h>

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

int sched_unit_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_data *data)
{
	switch (data->cmdcode) {
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

	LOOKUP_SYMS(tk_core);
	tk = &orig_tk_core->timekeeper;
	base = tk->tkr_mono.base;
	nsecs = tk->tkr_mono.xtime_nsec;
	now2 = ktime_add_ns(base, nsecs);

	now1 = ktime_get();

	printk("zz %s now1:%lx now2:%lx base:%lx\n",__func__, (unsigned long)now1, (unsigned long)now2, (unsigned long)base);

	return 0;
}

int sched_unit_exit(void)
{

	return 0;
}

