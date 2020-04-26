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


static int __init gpiodriver_init(void)
{
	printk("gpiodriver load \n");
	return 0;
}

static void __exit gpiodriver_exit(void)
{
	printk("gpiodriver unload \n");
}

//module_init(gpiodriver_init);
//module_exit(gpiodriver_exit);

//MODULE_LICENSE("GPL");
//MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
