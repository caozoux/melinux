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
#include <linux/syscore_ops.h>


static void hrtimer_entry_init(void)
{
struct hrtimer *hrtimer = &__raw_get_cpu_var(watchdog_hrtimer);
hrtimer_init(hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);)ti
}
static int __init hrtimerdriver_init(void)
{
	printk("hrtimerdriver load \n");
	return 0;
}

static void __exit hrtimerdriver_exit(void)
{
	printk("hrtimerdriver unload \n");
}

module_init(hrtimerdriver_init);
module_exit(hrtimerdriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
