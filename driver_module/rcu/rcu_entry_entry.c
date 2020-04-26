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


static int __init rcu_entrydriver_init(void)
{
	printk("rcu_entrydriver load \n");
	return 0;
}

static void __exit rcu_entrydriver_exit(void)
{
	printk("rcu_entrydriver unload \n");
}

module_init(rcu_entrydriver_init);
module_exit(rcu_entrydriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
