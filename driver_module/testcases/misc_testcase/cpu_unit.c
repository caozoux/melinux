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

#include <asm/stacktrace.h>
#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"
#include "medelay.h"

struct cpu_smp_data {
	unsigned long start;
	unsigned long end;
};

void cpu_wakeup_time(void *info)
{
	struct cpu_smp_data *data = (struct cpu_smp_data*) info;
	data->end = get_time_tick();
}

int cpu_unit_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_data *data)
{
	return 0;
}

int cpu_unit_init(void)
{
	int ret;
	struct cpu_smp_data data;
	int cpu = smp_processor_id();

	data.start = get_time_tick();
	ret = smp_call_function_single(cpu+1,
			cpu_wakeup_time,
			&data, 1);
	printk("cpu%d wake cpu%d time is %ld\n", cpu,  cpu+1, data.end - data.start);

	return 0;
}

int cpu_unit_exit(void)
{
	return 0;
}

