#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/bitops.h>

#include <ksysd.h>
#include <kpercpu.h>
#include "ksysdata.h"
#include "ksysd_ioctl.h"
#include "ioctl_kprobe.h"
#include "ktrace.h"
#include "ksioctl/ktrace_ioctl.h"

#if 0
static void trace_irq_handler_entry_hit(int irq, struct irqaction *action)
#endif

#define TRACE_DATA(trace)  int trace_##trace;

DECLARE_BITMAP(ktrace_event_bits, IOCTL_USEKTRACE_NR);

struct ktrace_event_item {
	char name[128];
	void *probe;
	void *data;
};

struct ktrace_data {
	struct ktrace_event_item *event_list;
}ktrace_d;

int ktrace_enable_tracepoint(const char *name, void *probe, void *data, enum IOCTL_USEKTRACE_SUB type)
{
	int ret;
	struct ktrace_event_item *event_item;

	if (test_and_set_bit(type,ktrace_event_bits ))
		return 0;

	ret = register_tracepoint(name, probe, data);
	if (ret) 
		goto out;

	event_item = &ktrace_d.event_list[(int)type];
	strncpy(event_item->name, name, 128);
	event_item->probe = probe;
	event_item->data = data;

	return 0;
out:
	clear_bit(type, ktrace_event_bits);
	return  ret;
}

int ktrace_disable_tracepoint(const char *name, void *probe, void *data, enum IOCTL_USEKTRACE_SUB type)
{
	int ret;

	if (test_and_clear_bit(type,ktrace_event_bits ))
		return 0;

	ret = unregister_tracepoint(name, probe, data);
	if (ret) 
		goto out;

	return 0;
out:
	set_bit(type, ktrace_event_bits);
	return  ret;
}

static void trace_sched_switch_entry_hit(void *__data, bool preempt, struct task_struct *prev, struct task_struct *next)
{
	//int cpu = smp_processor_id();
	if (prev->pid != 0) {
		struct sched_entity *se = &prev->se;
		struct percpu_data *pcpu_d = get_ksys_percpu();

		pcpu_d->stat.prev_task = next;
		trace_printk("prev:%s exec_runtime:%lld %lld %lld\n", prev->comm, se->sum_exec_runtime, se->prev_sum_exec_runtime, se->sum_exec_runtime - se->prev_sum_exec_runtime);
	}
	//trace_printk("prev:%s pid:%d next:%s \n", prev->comm, prev->pid, next->comm);
			
}


static int ktrace_unit_sched(bool enable)
{
	if (enable) {
		ktrace_enable_tracepoint("sched_switch", trace_sched_switch_entry_hit, NULL, IOCTL_USEKTRACE_SCHED_SWITCH);
		//register_tracepoint("sched_switch", trace_sched_switch_entry_hit, NULL);
	} else {
		ktrace_disable_tracepoint("sched_switch", trace_sched_switch_entry_hit, NULL, IOCTL_USEKTRACE_SCHED_SWITCH);
		//unregister_tracepoint("sched_switch", trace_sched_switch_entry_hit, NULL);
	}

	return 0;
}

static int ktrace_unit_rcu(bool enable)
{
#if 0
	int ret;
//	unregister_tracepoint(const char *name, void *probe, void *data)
	ret = register_tracepoint(const char *name, void *probe, void *data);
	register_tracepoint("irq_handler_entry", trace_irq_handler_entry_hit, NULL);
#endif
	return 0;
}

int ktrace_unit_ioctl_func(unsigned int cmd, unsigned long size, struct ioctl_ksdata *data)
{
	int ret = 0;
	struct ktrace_ioctl kioctl;

	DBG("subcmd:%d\n", (int)data->subcmd);
	if (copy_from_user(&kioctl, (char __user *)data->data, sizeof(struct ktrace_ioctl))) {
		printk("ioctl data copy err\n");
		ret = -EFAULT;
		goto OUT;
	}

	//printk("zz %s %d \n", __func__, __LINE__);
	switch (data->subcmd) {
		case IOCTL_USEKTRACE_RCU:
			ret = ktrace_unit_rcu(kioctl.enable);
			break;

		case IOCTL_USEKTRACE_SCHED_SWITCH:
			ret = ktrace_unit_sched(kioctl.enable);
			break;
		default:
			break;
	}

OUT:
	return ret;
}

int ktrace_unit_init(void)
{
	ktrace_d.event_list = (struct ktrace_event_item *) vmalloc(sizeof(struct ktrace_event_item) * IOCTL_USEKTRACE_NR);
	if (!ktrace_d.event_list)
		goto out;

	return 0;
out:
	return -EINVAL;
}

int ktrace_unit_exit(void)
{
	int bit, ret ;

	for_each_set_bit(bit, ktrace_event_bits, IOCTL_USEKTRACE_NR) {
		struct ktrace_event_item *event_item;
		event_item = &ktrace_d.event_list[(int)bit];
		printk("zz %s %d %s\n", __func__, __LINE__, event_item->name);
		ret = unregister_tracepoint(event_item->name, event_item->probe, event_item->data);
		if (ret)
			pr_err("%s free fialed\n", event_item->name);
	}

	if (ktrace_d.event_list)
		vfree((void*)ktrace_d.event_list);

	return 0;
}

