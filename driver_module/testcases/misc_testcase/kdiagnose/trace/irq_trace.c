#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/version.h>
#include <linux/tracepoint.h>

#include "../template_iocmd.h"
#include "../misc_ioctl.h"
#include "../debug_ctrl.h"
#include "mekernel.h"
#include "kmemlocal.h"
#include "ktrace.h"


#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)                                                                                                                                                                                          
static void trace_irq_handler_entry_hit(int irq,
        struct irqaction *action)
#else
static void trace_irq_handler_entry_hit(void *ignore, int irq,
                struct irqaction *action)
#endif  
{  
	
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
static void trace_irq_handler_exit_hit(int irq,
        struct irqaction *action, int ret)
#else
static void trace_irq_handler_exit_hit(void *ignore, int irq,
                struct irqaction *action, int ret)
#endif
{   

}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
static void trace_irq_handler_exit_hit(int irq,                                                                                                                                                                                          
        struct irqaction *action, int ret)
#else
static void trace_irq_handler_exit_hit(void *ignore, int irq,
                struct irqaction *action, int ret)
#endif
{

}


#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
static void trace_softirq_entry_hit(struct softirq_action *h,
    struct softirq_action *softirq_vec)
#else
static void trace_softirq_entry_hit(void *ignore, unsigned long nr_sirq)
#endif
{

}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
static void trace_softirq_exit_hit(struct softirq_action *h,
    struct softirq_action *softirq_vec)
#else
static void trace_softirq_exit_hit(void *ignore, unsigned long nr_sirq)
#endif  
{

}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
static void trace_timer_expire_entry_hit(struct timer_list *timer)
#else
static void trace_timer_expire_entry_hit(void *ignore, struct timer_list *timer)
#endif
{

}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
static void trace_timer_expire_exit_hit(struct timer_list *timer)
#else
static void trace_timer_expire_exit_hit(void *ignore, struct timer_list *timer)
#endif
{
    void *func = timer->function;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
static void trace_hrtimer_expire_entry_hit(struct hrtimer *timer, ktime_t *_now)
#else
static void trace_hrtimer_expire_entry_hit(void *ignore, struct hrtimer *timer, ktime_t *_now)
#endif
{

}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
static void trace_hrtimer_expire_exit_hit(struct hrtimer *timer)
#else
static void trace_hrtimer_expire_exit_hit(void *ignore, struct hrtimer *timer)
#endif
{

}

static init irq_trace_install(void)
{
	hook_tracepoint("irq_handler_entry", trace_irq_handler_entry_hit, NULL);
	hook_tracepoint("irq_handler_exit", trace_irq_handler_exit_hit, NULL);
	hook_tracepoint("softirq_entry", trace_softirq_entry_hit, NULL);
	hook_tracepoint("softirq_exit", trace_softirq_exit_hit, NULL);
	hook_tracepoint("timer_expire_entry", trace_timer_expire_entry_hit, NULL);
	hook_tracepoint("timer_expire_exit", trace_timer_expire_exit_hit, NULL);
	hook_tracepoint("hrtimer_expire_entry", trace_hrtimer_expire_entry_hit, NULL);
	hook_tracepoint("hrtimer_expire_exit", trace_hrtimer_expire_exit_hit, NULL);
}

static init irq_trace_remove(void)
{
	unhook_tracepoint("irq_handler_entry", trace_irq_handler_entry_hit, NULL);
	unhook_tracepoint("irq_handler_exit", trace_irq_handler_exit_hit, NULL);
	unhook_tracepoint("softirq_entry", trace_softirq_entry_hit, NULL);
	unhook_tracepoint("softirq_exit", trace_softirq_exit_hit, NULL);
	unhook_tracepoint("timer_expire_entry", trace_timer_expire_entry_hit, NULL);
	unhook_tracepoint("timer_expire_exit", trace_timer_expire_exit_hit, NULL);
	unhook_tracepoint("hrtimer_expire_entry", trace_hrtimer_expire_entry_hit, NULL);
	unhook_tracepoint("hrtimer_expire_exit", trace_hrtimer_expire_exit_hit, NULL);
}
