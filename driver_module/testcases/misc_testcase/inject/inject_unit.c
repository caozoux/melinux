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

static struct hrtimer inject_timer;


static enum hrtimer_restart inject_timer_func(struct hrtimer *timer)
{
	ktime_t now;
	int i;

	printk("zz %s now:%ld +\n",__func__, (unsigned long)ktime_get());
	//for (i = 0; i < 10000; ++i) {
	for (i = 0; i < 1000; ++i) {
		udelay(100);
	}
	printk("zz %s now:%ld -\n",__func__, (unsigned long)ktime_get());
 	now = ktime_get();
	//hrtimer_forward(timer, now, NSEC_PER_SEC / HZ);
	hrtimer_forward(timer, now, (NSEC_PER_SEC/HZ)/10);
	return HRTIMER_RESTART;
	//return HRTIMER_NORESTART;
}

// inject hrtime long runtime to cause hrtime delta
static void inject_hrtime_timeout_start(void)
{
	ktime_t kt;
	hrtimer_init(&inject_timer, CLOCK_MONOTONIC, HRTIMER_MODE_ABS);
	inject_timer.function = inject_timer_func;
	kt = ktime_add_us(ktime_get(), (NSEC_PER_SEC/HZ)/10 );

	hrtimer_set_expires(&inject_timer, kt);
	hrtimer_start_expires(&inject_timer, HRTIMER_MODE_ABS_PINNED);
	MEDEBUG("inject_hrtime_timeout_start\n");
}

static void inject_hrtime_timeout_exit(void)
{
	hrtimer_cancel(&inject_timer); 
	MEDEBUG("inject_hrtime_timeout_exit\n");
}

int inject_unit_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_data *data)
{
	void *inject_pointer = NULL;
	MEDEBUG("%s %d\n", __func__, data->cmdcode)
	switch (data->cmdcode) {
		case IOCTL_INJECT_NULL:
			printk("zz %s %d \n", __func__, __LINE__);
			MEDEBUG("inject NULL pointer");
			*(int *)inject_pointer = 0;
			break;
		case IOCTL_INJECT_WRITE_PROTECT:
			MEDEBUG("inject write protect");
			inject_pointer = inject_unit_ioctl_func;
			*(int *)inject_pointer = 0;
			break;
		case IOCTL_INJECT_MUTET_DEPLOCK:
			break;
		case IOCTL_INJECT_SPINLOCK_DEPLOCK:
			break;
		case IOCTL_INJECT_IRQSPINLOCK_DEPLOCK:
			break;
		case IOCTL_INJECT_RUC_HANG:
			break;
		case IOCTL_INJECT_SOFTWATCHDOG_TIMEOUT:
			break;
		default:
			break;
	}

	return 0;
}

int inject_unit_init(void)
{
	inject_hrtime_timeout_start();
	return 0;
}

int inject_unit_exit(void)
{
	inject_hrtime_timeout_exit();
	return 0;
}

