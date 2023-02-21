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

#define MAX_ALLOC   (1024)
#define ALLOC_SIZE  (PAGE_SIZE)
#define TIME_INTERVAL (1000000)

//struct cpumask watchdog_allowed_mask __read_mostly;
static DEFINE_PER_CPU(struct hrtimer, watchdog_hrtimer);
static DEFINE_PER_CPU(struct timer_list, watchdog_timer);
static DEFINE_PER_CPU(bool, perthread_run);
static DEFINE_PER_CPU(u64, perthread_ktime);
static DEFINE_PER_CPU(void*, per_malloc_list);

static DEFINE_PER_CPU(u64, permalloc_diff_total);
static DEFINE_PER_CPU(u64, permalloc_cnt);
unsigned long (*cust_kallsyms_lookup_name)(const char *name);
void (*orig_arch_trigger_cpumask_backtrace)(const cpumask_t *mask, bool exclude_self);

static int ctrl_hrtime = 0;
static int malloc_lantcy_cnt = 0;
struct hrtimer hrtimer_pr;

static enum hrtimer_restart hrtimer_pr_fun(struct hrtimer *hrtimer)
{
  	trace_printk("zz malloc_lantcy_cnt:%d \n", malloc_lantcy_cnt);
	malloc_lantcy_cnt = 0;
	hrtimer_forward_now(&hrtimer_pr, ns_to_ktime(1000000000));
	return HRTIMER_RESTART;
}
void malloc_lantcy_run(void *malloc_list)

{
	u64 old;
	u64 new = 0, diff;
	int i;
	void **buf = malloc_list;
	u64 *malloc_diff = this_cpu_ptr(&permalloc_diff_total);
	u64 *malloc_cnt = this_cpu_ptr(&permalloc_cnt);

	for (i = 0; i < MAX_ALLOC; ++i) {
		old =  ktime_get_ns();
		buf[i] = kmalloc(ALLOC_SIZE, GFP_ATOMIC);
		if (!buf[i])
			trace_printk("Err: PAGE_SIZE malloc failed\n");
		new = ktime_get_ns();
		diff = new - old;
		if (diff > 200000) {
			//trace_printk("new-old:%lld \n", (unsigned long)new-old);
			//trace_printk("normal:%lld \n", (unsigned long)*malloc_diff/(*malloc_cnt));
			malloc_lantcy_cnt += 1;
		}  else {
			*malloc_diff += diff;
			*malloc_cnt += 1;
		}

	}

	for (i = 0; i < MAX_ALLOC; ++i) {
		if (buf[i])
			kfree(buf[i]);
	}
}

//static struct check_regs __percpu *per_regs;
static enum hrtimer_restart watchdog_timer_fn(struct hrtimer *hrtimer)
{
	malloc_lantcy_run(this_cpu_ptr(&per_malloc_list));
	hrtimer_forward_now(hrtimer, ns_to_ktime(TIME_INTERVAL));
	return HRTIMER_RESTART;
}

static void ns_poll(struct timer_list *unused)
{
	struct timer_list *ns_timer = this_cpu_ptr(&watchdog_timer);
	void *p; 
	p = per_cpu(per_malloc_list, smp_processor_id());
	malloc_lantcy_run(p);
	//printk("zz %s val:%lx \n",__func__, (unsigned long)this_cpu_ptr(per_malloc_list));
	//mod_timer (ns_timer, jiffies + HZ);
	mod_timer (ns_timer, jiffies + 1);
}

static void watchdog(unsigned int cpu)
{
	bool *percpu_run = this_cpu_ptr(&perthread_run);

	if (ctrl_hrtime) {
		struct hrtimer *hrtimer = this_cpu_ptr(&watchdog_hrtimer);
		hrtimer_init(hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		hrtimer->function = watchdog_timer_fn;
		hrtimer_start(hrtimer, ns_to_ktime(TIME_INTERVAL),
				HRTIMER_MODE_REL_PINNED);
	} else {
		struct timer_list *ns_timer = this_cpu_ptr(&watchdog_timer);
		timer_setup(ns_timer, ns_poll, 0);
		ns_timer->expires = jiffies + 10;
		add_timer(ns_timer);
		printk("zz %s cpu:%lx \n",__func__, (unsigned long)cpu);
	}

	*percpu_run = false;

}

static void watchdog_enable(unsigned int cpu)
{
}

static void watchdog_cleanup(unsigned int cpu, bool online)
{
	if (ctrl_hrtime) {
		struct hrtimer *hrtimer = this_cpu_ptr(&watchdog_hrtimer);
		hrtimer_cancel(hrtimer);
	} else {
		struct timer_list *ns_timer = this_cpu_ptr(&watchdog_timer);
		del_timer_sync(ns_timer);
	}

}

static int watchdog_should_run(unsigned int cpu)
{
	bool *percpu_run = this_cpu_ptr(&perthread_run);
	return *percpu_run;
	//return false;
}

static void watchdog_disable(unsigned int cpu)
{
	if (ctrl_hrtime) {
		struct hrtimer *hrtimer = this_cpu_ptr(&watchdog_hrtimer);
		hrtimer_cancel(hrtimer);
	}
}

static struct smp_hotplug_thread watchdog_threads = {
	//.store          = &softlockup_watchdog,
	.thread_should_run  = watchdog_should_run,
	.thread_fn      = watchdog,
	.thread_comm        = "malloc_lantcy/%u",
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

static int hrtimer_pr_init(void)
{
	hrtimer_init(&hrtimer_pr, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hrtimer_pr.function = hrtimer_pr_fun;
	hrtimer_start(&hrtimer_pr, ns_to_ktime(1000000000),
			HRTIMER_MODE_REL_PINNED);

	return 0;
}

static void hrtimer_pr_exit(void)
{
	hrtimer_cancel(&hrtimer_pr);
}

static int __init percpu_hrtimer_init(void)
{
	int ret, cpu;
	u64 ns =  ktime_get_ns();

	for_each_online_cpu(cpu) {
		void *p;
		p = kmalloc(MAX_ALLOC*sizeof(void*),GFP_KERNEL);
		printk("zz %s p:%lx \n",__func__, (unsigned long)p);
		if (p)
			per_cpu(per_malloc_list, cpu) = p;
		else
			return -EINVAL;
	}

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

  	hrtimer_pr_init();	
	printk("zz %s %d \n", __func__, __LINE__);
	return 0;
}

static void __exit percpu_hrtimer_exit(void)
{
	int cpu;
	printk("zz %s %d \n", __func__, __LINE__);
	smpboot_unregister_percpu_thread(&watchdog_threads);
	for_each_online_cpu(cpu) {
		kfree(per_cpu(per_malloc_list, cpu));
	}
  	hrtimer_pr_exit();	
}

module_init(percpu_hrtimer_init);
module_exit(percpu_hrtimer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
