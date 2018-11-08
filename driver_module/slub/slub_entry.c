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


static int __init slubufadriver_init(void)
{
	printk("slubufadriver load \n");
	return 0;
}

static void __exit slubufadriver_exit(void)
{
	printk("slubufadriver unload \n");
}

module_init(slubufadriver_init);
module_exit(slubufadriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
