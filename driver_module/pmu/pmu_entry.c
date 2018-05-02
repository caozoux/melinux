#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/pm_runtime.h>
#include <linux/pm.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/irqdomain.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/memory.h>
#include <linux/mm_types.h>
#include <linux/platform_data/gpio-omap.h>
#include <asm/irq_regs.h>
#include <linux/kvm_para.h>
#include <linux/perf_event.h>
#include <asm/msr.h>

static struct perf_event *watchdog_ev;
static void rdpmc_test(void)
{
	u64 new_raw_count;
	//PERF_COUNT_HW_INSTRUCTIONS
	//base_rdpmc:40000000
	//rdpmcl(0, new_raw_count);
	new_raw_count=native_read_pmc(0);

	printk("zz %s %d new_raw_count:%lx \n",__func__, __LINE__, (unsigned long)new_raw_count);
}

unsigned long rdpmc_read(unsigned int c)
{
   unsigned a, d;
   __asm__ volatile("rdpmc" : "=a" (a), "=d" (d) : "c" (c));
   return ((unsigned long)a) | (((unsigned long)d) << 32);;
}

static struct perf_event_attr wd_hw_attr = {
    .type       = PERF_TYPE_HARDWARE,
    .config     = PERF_COUNT_HW_CPU_CYCLES,
    .size       = sizeof(struct perf_event_attr),
    .pinned     = 1,
    .disabled   = 1,
};

/* Callback function for perf event subsystem */
static void watchdog_overflow_callback(struct perf_event *event,
         struct perf_sample_data *data,
         struct pt_regs *regs)
{
    /* Ensure the watchdog never gets throttled */
    event->hw.interrupts = 0;
	printk("zz %s %d \n", __func__, __LINE__);
}

static void perf_event_test_init(void)
{
	struct perf_event_attr *wd_attr;
	struct perf_event *event;
	struct hw_perf_event *hwc;
	u64 enabled=0, running=0, val;
	int this_cpu = smp_processor_id();

	wd_attr = &wd_hw_attr;
	event = perf_event_create_kernel_counter(wd_attr, this_cpu, NULL, watchdog_overflow_callback, NULL);
	hwc = &event->hw;
	//perf_event_enable(event);
	watchdog_ev = event;


	//rdpmcl(hwc->event_base_rdpmc, val);
	//trace_printk("zz %s %d val:%lx \n",__func__, __LINE__, (unsigned long)val);

	//mdelay(10);
	val=perf_event_read_value(event, &enabled, &running);
	trace_printk("zz- %s %dval:%lx enabled:%lx running:%lx \n",__func__, __LINE__, (unsigned long)val, (unsigned long)enabled, (unsigned long)running);


	//val=native_read_pmc(0);
	//trace_printk("zz %s %d val:%lx \n",__func__, __LINE__, (unsigned long)val);
}

static void perf_event_test_exit(void)
{
	struct perf_event *event = watchdog_ev;

	perf_event_disable(event);
	/* should be in cleanup, but blocks oprofile */
	perf_event_release_kernel(event);
}

static int __init pmutest_init(void)
{
	unsigned long status;

#if 1
	rdmsrl(MSR_CORE_PERF_FIXED_CTR0, status);
	printk("zz %s %d  MSR_CORE_PERF_FIXED_CTR0:%lx \n",__func__, __LINE__, (unsigned long)status);

	rdmsrl(MSR_CORE_PERF_FIXED_CTR1, status);
	printk("zz %s %d  MSR_CORE_PERF_FIXED_CTR1:%lx \n",__func__, __LINE__, (unsigned long)status);

	rdmsrl(MSR_CORE_PERF_FIXED_CTR2, status);
	printk("zz %s %d  MSR_CORE_PERF_FIXED_CTR2:%lx \n",__func__, __LINE__, (unsigned long)status);

	rdmsrl(MSR_CORE_PERF_FIXED_CTR_CTRL, status);
	printk("zz %s %d  MSR_CORE_PERF_FIXED_CTR_CTRL:%lx \n",__func__, __LINE__, (unsigned long)status);

	rdmsrl(MSR_CORE_PERF_GLOBAL_STATUS, status);
	printk("zz %s %d MSR_CORE_PERF_GLOBAL_STATUS:%lx \n",__func__, __LINE__, (unsigned long)status);

#else
	rdmsrl(MSR_CORE_PERF_GLOBAL_CTRL, status);
	printk("zz %s %d  MSR_CORE_PERF_GLOBAL_CTRL:%lx \n",__func__, __LINE__, (unsigned long)status);
	printk("zz %s \n", __func__);
#endif

	//perf_event_test_init();
	rdpmc_test();
	return 0;
}

static void __exit pmutest_exit(void)
{
	//perf_event_test_exit();
	printk("zz %s \n", __func__);
}

module_init(pmutest_init);
module_exit(pmutest_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Luciano Coelho <coelho@ti.com>");
MODULE_AUTHOR("Juuso Oikarinen <juuso.oikarinen@nokia.com>");
