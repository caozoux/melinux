#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/clocksource.h>
#include <linux/syscore_ops.h>
#include <linux/syscalls.h>

static struct list_head *orig_clocksource_list;
static struct clocksource **orig_curr_clocksource;
static int orig_finished_booting;
static struct work_struct *orig_watchdog_work;
static void (*orig_timekeeping_notify)(struct clocksource *clock);
struct timer_list *orig_watchdog_timer;
//clocksource_mutex
//timekeeping_notify

//timekeeping_notify(curr_clocksource);

static int disable_clocksource(struct clocksource *cs)
{
	cs->flags &= ~(CLOCK_SOURCE_VALID_FOR_HRES | CLOCK_SOURCE_WATCHDOG);
	cs->flags |= CLOCK_SOURCE_UNSTABLE;
#if 0
	if (orig_finished_booting)
		schedule_work(&orig_watchdog_work);
#endif
	printk("disable cs:%s\n", cs->name);
	return 0;
}

static int enable_clocksource(struct clocksource *cs)
{
	cs->flags |= (CLOCK_SOURCE_VALID_FOR_HRES | CLOCK_SOURCE_WATCHDOG);
	cs->flags &= ~CLOCK_SOURCE_UNSTABLE;
	*orig_curr_clocksource = cs;
	orig_timekeeping_notify(*orig_curr_clocksource);
	printk("set current clocksource: %s\n", (*orig_curr_clocksource)->name);
	return 0;

}

static int switch_clocksource(int index)
{
	struct clocksource *cs;
	if (list_empty(orig_clocksource_list)) {
		return 1;
	}

	list_for_each_entry(cs, orig_clocksource_list, list) {
		printk("zz %s cs->name:%s \n",__func__, cs->name);
		if (strstr("tsc", cs->name)) {
			//disable_clocksource(cs);
			enable_clocksource(cs);
		}
	}
	return 0;
}

static int symbol_init(void)
{
	orig_clocksource_list = (struct list_head *)kallsyms_lookup_name("clocksource_list");
	if (!orig_clocksource_list) {
		printk("find sysmbol clocksource_list failed\n");
		return 1;
	}

	orig_finished_booting= (int)kallsyms_lookup_name("finished_booting");
	if (!orig_finished_booting) {
		printk("find sysmbol orig_finished_booting failed\n");
		return 1;
	}

	orig_curr_clocksource = (struct clocksource*)kallsyms_lookup_name("curr_clocksource");
	if (!orig_curr_clocksource) {
		printk("find sysmbol orig_curr_clocksource failed\n");
		return 1;
	}

	orig_timekeeping_notify = (struct clocksource*)kallsyms_lookup_name("timekeeping_notify");
	if (!orig_timekeeping_notify) {
		printk("find sysmbol orig_timekeeping_notify failed\n");
		return 1;
	}

#if 0
	orig_watchdog_timer = (struct clocksource*)kallsyms_lookup_name("clocksource_enqueue_watchdog");
	if (!orig_watchdog_timer) {
		printk("find sysmbol orig_watchdog_timer failed\n");
		return 1;
	}
	orig_watchdog_work = (struct work_struct*)kallsyms_lookup_name("watchdog_work");
	if (!orig_watchdog_work) {
		printk("find sysmbol orig_watchdog_work failed\n");
		return 1;
	}
#endif

	return 0;
}

static int __init clocksourcedriver_init(void)
{
	if (symbol_init()) {
		return 1;
	}

	printk("clocksourcedriver load \n");

	switch_clocksource(0);

	return 0;
}

static void __exit clocksourcedriver_exit(void)
{
	printk("clocksourcedriver unload \n");
}

module_init(clocksourcedriver_init);
module_exit(clocksourcedriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
