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

struct tick_sched  *orig_tick_cpu_sched;
struct tick_sched  *cpu0_tick_cpu_sched;


int tick_sched_init(void)
{
	LOOKUP_SYMS(tick_cpu_sched);
	cpu0_tick_cpu_sched = per_cpu(orig_tick_cpu_sched, 0);
	return 0;
}

void tick_sched_exit(void)
{

}

