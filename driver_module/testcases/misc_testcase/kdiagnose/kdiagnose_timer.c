#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/version.h>

#include <kdiagnose_timer.h>
#include "../template_iocmd.h"
#include "../misc_ioctl.h"
#include "../debug_ctrl.h"
#include "mekernel.h"
#include "kdiagnose.h"
#include "ktrace.h"

static struct hrtimer monitor_timer;

struct list_head dg_mt_header;

// monitor callback func

static enum hrtimer_restart monitor_timer_func(struct hrtimer *timer)
{
	ktime_t now;
	int i;

 	now = ktime_get();
	hrtimer_forward(timer, now, NSEC_PER_SEC/HZ);

	return HRTIMER_RESTART;
}

int register_hrtime_moninter(struct dg_mt_timer *timer)
{
	list_add_tail(&timer->list, &dg_mt_header);
	return 0;
}

int remove_hrtime_moninter(struct dg_mt_timer *timer)
{
	list_del(&timer->list);
	return 0;
}

int diagnose_hrtime_moninter(void)
{
	ktime_t kt;
	struct dg_mt_timer *p, *n;

	hrtimer_init(&monitor_timer, CLOCK_MONOTONIC, HRTIMER_MODE_ABS);
	monitor_timer.function = monitor_timer_func;
	kt = ktime_add_us(ktime_get(), (NSEC_PER_SEC/HZ));

	if (list_empty(&dg_mt_header))
		goto out;

	list_for_each_entry_safe(p, n, &dg_mt_header, list) {
		printk("zz %s p:%lx \n",__func__, (unsigned long)p);
	}

out:
	hrtimer_set_expires(&monitor_timer, kt);
	hrtimer_start_expires(&monitor_timer, HRTIMER_MODE_ABS_PINNED);

	MEDEBUG("monitor timer start\n");
	return 0;
}

int diagnose_hrtime_init(void)
{
	//INIT_LIST_HEAD(dg_mt_header);
	diagnose_hrtime_moninter();
	return 0;
}

