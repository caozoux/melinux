#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/kprobes.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/smpboot.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/nmi.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>

//struct cpumask watchdog_allowed_mask __read_mostly;
static DEFINE_PER_CPU(struct hrtimer, watchdog_hrtimer);
static DEFINE_PER_CPU(bool, perthread_run);
static DEFINE_PER_CPU(u64, perthread_ktime);

unsigned long (*cust_kallsyms_lookup_name)(const char *name);
void (*orig_arch_trigger_cpumask_backtrace)(const cpumask_t *mask, bool exclude_self);

//static struct check_regs __percpu *per_regs;
static enum hrtimer_restart watchdog_timer_fn(struct hrtimer *hrtimer)
{
	int cpu;
	u64 *percpu_ns = this_cpu_ptr(&perthread_ktime);
	for_each_online_cpu(cpu) {
		u64 ns =  ktime_get_ns();
		u64 old = per_cpu(perthread_ktime, cpu);
		if (ns > (old + 200000000)) {
			printk("overtimer out %llx %llx cpu:%d \n", ns, old, smp_processor_id());
			per_cpu(perthread_ktime, cpu) = ns;
			orig_arch_trigger_cpumask_backtrace(cpumask_of(cpu), false);
		}
	}

	*percpu_ns = ktime_get_ns();
	hrtimer_forward_now(hrtimer, ns_to_ktime(10000000));
	return HRTIMER_RESTART;
}

static void watchdog(unsigned int cpu)
{
	struct hrtimer *hrtimer = this_cpu_ptr(&watchdog_hrtimer);
	bool *percpu_run = this_cpu_ptr(&perthread_run);

	hrtimer_init(hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hrtimer->function = watchdog_timer_fn;
	hrtimer_start(hrtimer, ns_to_ktime(10000000),
			HRTIMER_MODE_REL_PINNED);

	*percpu_run = false;

	printk("percpu hrtimer-%d start \n", cpu);

}

static void watchdog_enable(unsigned int cpu)
{
	printk("per cpu start \n");
}

static void watchdog_cleanup(unsigned int cpu, bool online)
{
	struct hrtimer *hrtimer = this_cpu_ptr(&watchdog_hrtimer);

	hrtimer_cancel(hrtimer);

}

static int watchdog_should_run(unsigned int cpu)
{
	bool *percpu_run = this_cpu_ptr(&perthread_run);
	printk("zz %s cpu:%lx \n",__func__, (unsigned long)cpu);
	return *percpu_run;
	//return false;
}

static void watchdog_disable(unsigned int cpu)
{
	struct hrtimer *hrtimer = this_cpu_ptr(&watchdog_hrtimer);

	hrtimer_cancel(hrtimer);

	printk("zz %s %d \n", __func__, __LINE__);
}

static struct smp_hotplug_thread watchdog_threads = {
	//.store          = &softlockup_watchdog,
	.thread_should_run  = watchdog_should_run,
	.thread_fn      = watchdog,
	.thread_comm        = "watchdog/%u",
	.setup          = watchdog_enable,
	.cleanup        = watchdog_cleanup,
	.park           = watchdog_disable,
	.unpark         = watchdog_enable,
	//.selfparking        = true,
};

static int (*ksys_kallsyms_on_each_symbol)(int (*fn)(void *, const char *,
        struct module *, unsigned long),void *data);

static int symbol_walk_callback(void *data, const char *name,
        struct module *mod, unsigned long addr)
{
    if (strcmp(name, "kallsyms_lookup_name") == 0) {
        cust_kallsyms_lookup_name = (void *)addr;
        return addr;
    }

    return 0;
}

static int get_kallsyms_lookup_name(void)
{
    int ret;
    ksys_kallsyms_on_each_symbol = &kallsyms_on_each_symbol;
    ret = ksys_kallsyms_on_each_symbol(symbol_walk_callback, NULL);
    if (!ret || !cust_kallsyms_lookup_name)
        return -EINVAL;

    return 0;
}

int sym_init(void)
{

	orig_arch_trigger_cpumask_backtrace = (void *)cust_kallsyms_lookup_name("arch_trigger_cpumask_backtrace");
	if (!orig_arch_trigger_cpumask_backtrace)
		return -EINVAL;

	return 0;
}

static int __init percpu_hrtimer_init(void)
{
	int ret, cpu;
	u64 ns =  ktime_get_ns();

	if (get_kallsyms_lookup_name())
		return -EINVAL;

	if (sym_init())
		return -EINVAL;

	for_each_online_cpu(cpu) {
		per_cpu(perthread_run, cpu) = true;
		per_cpu(perthread_ktime, cpu) = ns;
	}

	ret = smpboot_register_percpu_thread(&watchdog_threads);
	if (ret)
		return -EINVAL;

	printk("zz %s %d \n", __func__, __LINE__);
	return 0;
}

static void __exit percpu_hrtimer_exit(void)
{
	printk("zz %s %d \n", __func__, __LINE__);
	smpboot_unregister_percpu_thread(&watchdog_threads);
}

module_init(percpu_hrtimer_init);
module_exit(percpu_hrtimer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
