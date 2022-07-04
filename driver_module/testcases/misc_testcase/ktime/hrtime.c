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

extern struct hrtimer_cpu_base *orig_hrtimer_bases;

static void print_name_offset(void *sym)
{
	trace_printk("<%pK>\n", sym);
}

static void
print_timer(struct hrtimer *taddr, struct hrtimer *timer,
        int idx, u64 now)
{
    trace_printk(" #%d: \n", idx);
    print_name_offset(taddr);
    print_name_offset(timer->function);
    trace_printk(", S:%02x \n", timer->state);
    trace_printk(" # expires at %Lu-%Lu nsecs [in %Ld to %Ld nsecs]\n",
        (unsigned long long)ktime_to_ns(hrtimer_get_softexpires(timer)),
        (unsigned long long)ktime_to_ns(hrtimer_get_expires(timer)),
        (long long)(ktime_to_ns(hrtimer_get_softexpires(timer)) - now),
        (long long)(ktime_to_ns(hrtimer_get_expires(timer)) - now));
}

static void
print_active_timers(struct hrtimer_clock_base *base, u64 now)
{
    struct hrtimer *timer, tmp;
    unsigned long next = 0, i;
    struct timerqueue_node *curr;
    unsigned long flags;

next_one:
    i = 0;

    raw_spin_lock_irqsave(&base->cpu_base->lock, flags);

    curr = timerqueue_getnext(&base->active);
    /*  
     * Crude but we have to do this O(N*N) thing, because
     * we have to unlock the base when printing:
     */
    while (curr && i < next) {
        curr = timerqueue_iterate_next(curr);
        i++;
    }   

    if (curr) {
        timer = container_of(curr, struct hrtimer, node);
        tmp = *timer;
        raw_spin_unlock_irqrestore(&base->cpu_base->lock, flags);

        print_timer(timer, &tmp, i, now);
        next++;
        goto next_one;
    }
    raw_spin_unlock_irqrestore(&base->cpu_base->lock, flags);
}

static void
print_base(struct hrtimer_clock_base *base, u64 now)
{
    trace_printk("  .base:       %pK\n", base);
    trace_printk("  .index:      %d\n", base->index);
    trace_printk("  .resolution: %u nsecs\n", hrtimer_resolution);
    trace_printk(  "  .get_time:   \n");
    print_name_offset(base->get_time);
    trace_printk(  "\n");
#ifdef CONFIG_HIGH_RES_TIMERS
    trace_printk("  .offset:     %Lu nsecs\n",
           (unsigned long long) ktime_to_ns(base->offset));
#endif
    trace_printk(  "active timers:\n");
    print_active_timers(base, now + ktime_to_ns(base->offset));
}

void print_cpu_hrtimer(int cpu)
{
	 struct hrtimer_cpu_base *cpu_base = &per_cpu(*orig_hrtimer_bases, cpu);
	 u64 now = ktime_to_ns(ktime_get());
	 int i;

	 printk("zz %s %d \n", __func__, __LINE__);
	 for (i = 0; i < HRTIMER_MAX_CLOCK_BASES; i++) {
			print_base(cpu_base->clock_base + i, now); 	
	 }
}

