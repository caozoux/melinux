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
#include <linux/syscore_ops.h>
#include <linux/syscalls.h>
#include  "rcu_module.h"

static struct rcu_state *orig_rcu_state_p;

static int symbol_init(void)
{
	orig_rcu_state_p = (struct list_head *)kallsyms_lookup_name("rcu_state_p");
	if (!orig_rcu_state_p) {
		printk("find sysmbol rcu_state_p failed\n");
		return 1;
	}
	return 0;
}

static int __init rcudriver_init(void)
{
	if (symbol_init()) {
		return 1;	
	}
	printk("rcudriver load \n");
	return 0;
}

static void __exit rcudriver_exit(void)
{
	printk("rcudriver unload \n");
}

module_init(rcudriver_init);
module_exit(rcudriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
