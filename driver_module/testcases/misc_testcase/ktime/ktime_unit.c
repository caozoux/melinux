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

#include <asm/stacktrace.h>
#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"
#include "medelay.h"
#include "mekernel.h"
#include "ktimelocal.h"

struct list_head *orig_clockevent_devices;

static void timer_clockevent_device_scan(void)
{
	struct clock_event_device *dev;

	list_for_each_entry(dev, orig_clockevent_devices, list) {
		int cpu;
		printk("clockevent:%s\n", dev->name);
		for_each_cpu(cpu, dev->cpumask) {
			printk("bind into cpu%d \n", cpu);
		}
	}
}

int ktime_unit_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_data *data)
{
	switch (data->cmdcode) {
		case IOCTL_USEKTIME_DEV_SCAN:
			timer_clockevent_device_scan();
			break;

		default:
			break;
	}
	return 0;
}

int ktime_unit_init(void)
{
	LOOKUP_SYMS(clockevent_devices);
	tick_sched_init();
	print_cpu_hrtimer(0);

	return 0;
}

int ktime_unit_exit(void)
{
	tick_sched_exit();
	//*orig_virtio_queue_rq_hook = NULL;
	return 0;
}

