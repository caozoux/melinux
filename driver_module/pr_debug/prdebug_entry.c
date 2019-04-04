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
#include <linux/mmzone.h>
#include <linux/init.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <asm/current.h>
#include <asm/futex.h>


static int __init pagedriver_init(void)
{
	//printk("pagedriver load \n");
	pr_debug("pr3 pagedriver load \n");
	return 0;
}

static void __exit pagedriver_exit(void)
{
	//printk("pagedriver unload \n");
	pr_debug("pr4 pagedriver unload \n");
}

module_init(pagedriver_init);
module_exit(pagedriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
