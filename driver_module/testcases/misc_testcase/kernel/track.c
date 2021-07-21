#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/swap.h>
#include <linux/pci.h>
#include <linux/msi.h>
#include <linux/irq.h>
#include <linux/list.h>

#ifdef CONFIG_ARM64
#include <linux/irqchip/arm-gic-common.h>
#include <linux/irqchip/arm-gic-v3.h>
#endif
#include "../template_iocmd.h"
#include "../misc_ioctl.h"
#include "../debug_ctrl.h"
#include "mekernel.h"

#if 1
void print_track(const char *s, struct track *t, unsigned long pr_time)
{
	int i;
	pr_err("INFO: %s in %pS age=%lu cpu=%u pid=%d\n",
			s, (void *)t->addr, pr_time - t->when, t->cpu, t->pid);
	for (i = 0; i <  TRACK_ADDRS_COUNT; i++) {
		if (t->addrs[i])
			printk("zz %s addres:%pB \n",__func__, (void*)t->addrs[i]);
		else
			break;
	}
}
#endif
int get_current_track(struct track *track, unsigned long addr)
{
#ifndef CONFIG_STACKTRACE
	return -EINVLID;
#else
	struct stack_trace trace;
	int i;

	trace.nr_entries = 0;
	trace.max_entries = TRACK_ADDRS_COUNT;
	trace.entries = track->addrs;
	trace.skip = 0;
	kasan_disable_current();
	save_stack_trace(&trace);
	kasan_enable_current();
	if (trace.nr_entries != 0 &&
			trace.entries[trace.nr_entries - 1] == ULONG_MAX)
		trace.nr_entries--;
	for (i = trace.nr_entries; i < TRACK_ADDRS_COUNT; i++)
		track->addrs[i] = 0;
	track->addr = addr;
	track->cpu = smp_processor_id();
	track->pid = current->pid;
	track->when = jiffies;
#endif
	return 0;
}

