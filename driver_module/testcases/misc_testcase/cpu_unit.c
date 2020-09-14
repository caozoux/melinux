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
#include <linux/blk-mq.h>

#include <asm/stacktrace.h>
#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"
#include "medelay.h"
#include "mekernel.h"

bool (*orig_cpus_share_cache)(int this_cpu, int that_cpu);

int cpu_unit_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_data *data)
{
	
	switch (data->cmdcode) {
		default:
			break;
	}

	return 0;
}

int cpu_unit_init(void)
{
	int cpu, cpu2;
	int share;

	LOOKUP_SYMS(cpus_share_cache);
	//printk("zz %s\n", __func__);
#if 0
	cpu = 1;
	share = orig_cpus_share_cache(cpu, cpu+1);
	if (share)
		printk("cpu%d and cpu%d is cache\n", cpu, cpu+1);
	else
		printk("cpu%d and cpu%d is not cache\n", cpu, cpu+1);
#else
#if 0
	for_each_possible_cpu(cpu) {
		for_each_possible_cpu(cpu2) {
			if (cpu == cpu2)
				continue;
			share = orig_cpus_share_cache(cpu, cpu2);
			if (cpu_online(cpu+1)) {
				if (share)
					printk("cpu%d and cpu%d is cache\n", cpu, cpu2);
				else
					printk("cpu%d and cpu%d is not cache\n", cpu, cpu2);
			}
		}
	}
#endif
#endif

	return 0;
}

int cpu_unit_exit(void)
{
	return 0;
}

