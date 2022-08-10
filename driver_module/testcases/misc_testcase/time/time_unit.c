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

#include <asm/stacktrace.h>
#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"
#include "mekernel.h"
#include "medelay.h"

#define CLOCK_TO_NS(cs) (u64) ((cs->read(cs) * cs->mult) >> cs->shift)
static struct list_head *orig_clocksource_list;

static void dump_clocksource_emun(void)
{
	struct clocksource *cs;
	list_for_each_entry(cs, orig_clocksource_list, list) {
		printk("%s rating:%d mult:%x shift:%x\n", cs->name
				,cs->rating
				,cs->mult
				,cs->shift
				);
	}
}

int time_unit_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_data *data)
{
	return 0;
}

int time_unit_init(void)
{
	LOOKUP_SYMS(clocksource_list);

	if (init_dump_info)
		dump_clocksource_emun();

	return 0;
}

int time_unit_exit(void)
{
	return 0;
}

