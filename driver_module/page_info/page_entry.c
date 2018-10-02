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


static int __init pagedriver_init(void)
{
	struct page *page;
	void *addr;
	unsigned long pfn;
	addr=kmalloc(4096, GFP_KERNEL);
	page=virt_to_page(addr);
	pfn=page_to_pfn(page);
	printk("page:%lx addr:%lx pfn:%lx\n", page, addr, pfn);
	printk("page:%lx addr:%lx pfn:%lx\n", page, addr, PHYS_PFN_OFFSET);
	kfree(addr);
	printk("pagedriver load \n");
	return 0;
}

static void __exit pagedriver_exit(void)
{
	printk("pagedriver unload \n");
}

module_init(pagedriver_init);
module_exit(pagedriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
