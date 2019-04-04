#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/kallsyms.h>

void (*orig_show_free_areas)(unsigned int flags);
static int __init memorydriver_init(void)
{
	printk("gpiodriver load \n");
 	orig_show_free_areas = kallsyms_lookup_name("show_free_areas");
	if (orig_show_free_areas)
		orig_show_free_areas(0);
	printk("zz %s orig_show_free_areas:%lx \n",__func__, orig_show_free_areas);
	return 0;
}

static void __exit memorydriver_exit(void)
{
	printk("gpiodriver unload \n");
}

module_init(memorydriver_init);
module_exit(memorydriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
