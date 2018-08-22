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
#include "kpatch.h"

static struct kpatch_module kpmod;
static bool replace;
static int __init kpatchtestdriver_init(void)
{
	kpatch_register(&kpmod, replace);
	kpatch_unregister(&kpmod);
	printk("kpatchtestdriver load \n");
	return 0;
}

static void __exit kpatchtestdriver_exit(void)
{
	printk("kpatchtestdriver unload \n");
}

module_init(kpatchtestdriver_init);
module_exit(kpatchtestdriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
