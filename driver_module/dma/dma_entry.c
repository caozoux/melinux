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
#include <dma_alloc_test.h>

static void dma_test_init(void)
{
	struct page *page;
	page = alloc_pages(GFP_KERNEL | GFP_DMA, 2);
	page = __alloc_pages_nodemask(GFP_KERNEL | GFP_DMA, 1, 0, NULL);
	printk("addr:%lx\n", page);
}

static void dma_test_exit(void) 
{

}

static int __init gpiodriver_init(void)
{
	//dma_test_init();
	dma_alloc_test_init();
	printk("gpiodriver load \n");
	return 0;
}

static void __exit gpiodriver_exit(void)
{
	dma_alloc_test_exit();
	printk("gpiodriver unload \n");
}

module_init(gpiodriver_init);
module_exit(gpiodriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
