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
#include "ktrace.h"
#include "../kdiagnose.h"


extern struct softirq_action *orig_softirq_vec;
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)                                                                                                                                                                                          
static void trace_irq_handler_entry_hit(int irq,
        struct irqaction *action)
#else
static void trace_irq_handler_entry_hit(void *ignore, int irq,
                struct irqaction *action)
#endif  
{  
	struct kd_percpu_data  *kd_percpu_data;
	struct trace_irq  *trace_irq;
	time_avg *avg;
	u64 now;

	kd_percpu_data = get_kd_percpu_data();
	if (!kd_percpu_data)
		goto failed;

	trace_irq = &kd_percpu_data->trace_hwirq[irq];
	avg = &trace_irq->t_avg;

	now = ktime_to_ns(ktime_get());
	trace_irq[irq].t_record.start = now;

failed:
	return;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
static void trace_irq_handler_exit_hit(int irq,
        struct irqaction *action, int ret)
#else
static void trace_irq_handler_exit_hit(void *ignore, int irq,
                struct irqaction *action, int ret)
#endif
{
	struct kd_percpu_data  *kd_percpu_data;
	struct trace_irq  *trace_irq;
	time_avg *avg;
	u64 diff, end;

	kd_percpu_data = get_kd_percpu_data();
	if (!kd_percpu_data)
		goto failed;

	trace_irq = &kd_percpu_data->trace_hwirq[irq];
	avg = &trace_irq->t_avg;

	end = ktime_to_ns(ktime_get());
	trace_irq[irq].t_record.end = end;
	diff = end - trace_irq[irq].t_record.start;

	if (diff < avg->min || avg->min == 0)
		avg->min = diff;

	if (diff > avg->max) {
		avg->max = diff;
	}

	if (diff > 500000) {
		printk("hwirq diff:%lld \n", diff);
		printk("%pB\n", action->handler);
		dump_stack();
	}
	avg->g_cnt++;

failed:
	return;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
static void trace_softirq_entry_hit(struct softirq_action *h,
    struct softirq_action *softirq_vec)
#else
static void trace_softirq_entry_hit(void *ignore, unsigned long nr_sirq)
#endif
{
	struct kd_percpu_data  *kd_percpu_data;
	struct trace_irq  *trace_irq;
	time_avg *avg;
	u64 now;

	kd_percpu_data = get_kd_percpu_data();
	if (!kd_percpu_data)
		goto failed;

	trace_irq = &kd_percpu_data->trace_softirq[nr_sirq];
	avg = &trace_irq->t_avg;

	now = ktime_to_ns(ktime_get());
	trace_irq[nr_sirq].t_record.start = now;

failed:
	return;

}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
static void trace_softirq_exit_hit(struct softirq_action *h,
    struct softirq_action *softirq_vec)
#else
static void trace_softirq_exit_hit(void *ignore, unsigned long nr_sirq)
#endif  
{
    void *func;
	struct kd_percpu_data  *kd_percpu_data;
	struct trace_irq  *trace_irq;
	struct softirq_action *h;
	time_avg *avg;
	u64 diff, end;

	kd_percpu_data = get_kd_percpu_data();
	if (!kd_percpu_data)
		goto failed;

	trace_irq = &kd_percpu_data->trace_softirq[nr_sirq];
	avg = &trace_irq->t_avg;
	h = orig_softirq_vec + nr_sirq;
	func = h->action;

	end = ktime_to_ns(ktime_get());
	trace_irq[nr_sirq].t_record.end = end;
	diff = end - trace_irq[nr_sirq].t_record.start;

	if (diff < avg->min || avg->min == 0)
		avg->min = diff;

	if (diff > avg->max) {
		avg->max = diff;
	}

	if (diff > 500000) {
		printk("softirq diff:%lld \n", diff);
		printk("%pB\n", func);
		dump_stack();
	}
	avg->g_cnt++;

failed:
	return;

}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
static void trace_timer_expire_entry_hit(struct timer_list *timer)
#else
static void trace_timer_expire_entry_hit(void *ignore, struct timer_list *timer)
#endif
{
	struct kd_percpu_data  *kd_percpu_data;
	struct trace_timer  *trace_timer;
	u64 now;

	kd_percpu_data = get_kd_percpu_data();
	if (!kd_percpu_data)
		goto failed;

	trace_timer= &kd_percpu_data->trace_softtimer;

	now = ktime_to_ns(ktime_get());
	trace_timer->t_record.start = now;
failed:
	return;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
static void trace_timer_expire_exit_hit(struct timer_list *timer)
#else
static void trace_timer_expire_exit_hit(void *ignore, struct timer_list *timer)
#endif
{
    void *func = timer->function;
	struct kd_percpu_data  *kd_percpu_data;
	struct trace_timer  *trace_timer;
	time_avg *avg;
	u64 diff, end;

	kd_percpu_data = get_kd_percpu_data();
	if (!kd_percpu_data)
		goto failed;

	trace_timer= &kd_percpu_data->trace_softtimer;
	avg = &trace_timer->t_avg;

	end = ktime_to_ns(ktime_get());
	trace_timer->t_record.end = end;

	diff = end - trace_timer->t_record.start;

	if (diff < avg->min || avg->min == 0)
		avg->min = diff;

	if (diff > avg->max) {
		avg->max = diff;
	}

	if (diff > 500000) {
		printk("softtimer diff:%lld \n", diff);
		printk("%pB\n", func);
		dump_stack();
	}
failed:
	return;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
static void trace_hrtimer_expire_entry_hit(struct hrtimer *timer, ktime_t *_now)
#else
static void trace_hrtimer_expire_entry_hit(void *ignore, struct hrtimer *timer, ktime_t *_now)
#endif
{
	struct kd_percpu_data  *kd_percpu_data;
	struct trace_timer  *trace_timer;
	u64 now;

	kd_percpu_data = get_kd_percpu_data();
	if (!kd_percpu_data)
		goto failed;

	trace_timer= &kd_percpu_data->trace_hrtimer;

	now = ktime_to_ns(ktime_get());
	trace_timer->t_record.start = now;
failed:
	return;

}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
static void trace_hrtimer_expire_exit_hit(struct hrtimer *timer)
#else
static void trace_hrtimer_expire_exit_hit(void *ignore, struct hrtimer *timer)
#endif
{
    void *func = timer->function;
	struct kd_percpu_data  *kd_percpu_data;
	struct trace_timer  *trace_timer;
	time_avg *avg;
	u64 diff, end;

	kd_percpu_data = get_kd_percpu_data();
	if (!kd_percpu_data)
		goto failed;

	trace_timer= &kd_percpu_data->trace_hrtimer;
	avg = &trace_timer->t_avg;

	end = ktime_to_ns(ktime_get());
	trace_timer->t_record.end = end;

	diff = end - trace_timer->t_record.start;

	if (diff < avg->min || avg->min == 0)
		avg->min = diff;

	if (diff > avg->max) {
		avg->max = diff;
	}

	if (diff > 500000) {
		printk("%pB\n", func);
		printk("hrtimer diff:%lld \n", diff);
		dump_stack();
	}
failed:
	return;

}

int irq_trace_install(void)
{
	register_tracepoint("irq_handler_entry", trace_irq_handler_entry_hit, NULL);
	register_tracepoint("irq_handler_exit", trace_irq_handler_exit_hit, NULL);
	register_tracepoint("softirq_entry", trace_softirq_entry_hit, NULL);
	register_tracepoint("softirq_exit", trace_softirq_exit_hit, NULL);
	register_tracepoint("timer_expire_entry", trace_timer_expire_entry_hit, NULL);
	register_tracepoint("timer_expire_exit", trace_timer_expire_exit_hit, NULL);
	register_tracepoint("hrtimer_expire_entry", trace_hrtimer_expire_entry_hit, NULL);
	register_tracepoint("hrtimer_expire_exit", trace_hrtimer_expire_exit_hit, NULL);
	return 0;
}

int irq_trace_remove(void)
{
	unregister_tracepoint("irq_handler_entry", trace_irq_handler_entry_hit, NULL);
	unregister_tracepoint("irq_handler_exit", trace_irq_handler_exit_hit, NULL);
	unregister_tracepoint("softirq_entry", trace_softirq_entry_hit, NULL);
	unregister_tracepoint("softirq_exit", trace_softirq_exit_hit, NULL);
	unregister_tracepoint("timer_expire_entry", trace_timer_expire_entry_hit, NULL);
	unregister_tracepoint("timer_expire_exit", trace_timer_expire_exit_hit, NULL);
	unregister_tracepoint("hrtimer_expire_entry", trace_hrtimer_expire_entry_hit, NULL);
	unregister_tracepoint("hrtimer_expire_exit", trace_hrtimer_expire_exit_hit, NULL);
	return 0;
}

