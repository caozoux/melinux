#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/kallsyms.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>

struct mm_struct *orig_init_mm;

static int __init gpiodriver_init(void)
{
	unsigned long address;
	p4d_t *p4d;
	pgd_t *pgd;

	orig_init_mm = (void*) kallsyms_lookup_name("init_mm");
	if (!orig_init_mm)
		return -EINVAL;

	address = (unsigned long)kmalloc(32, GFP_KERNEL);
	printk("zz %s address:%lx index:%lx %lx\n",__func__, (unsigned long)address, (unsigned long) pgd_index((address)), orig_init_mm->pgg);

    pgd = pgd_offset(orig_init_mm, (address));
	if (pgd_none(*pgd))
		return -1;
	printk("zz %s pgd:%lx \n",__func__, (unsigned long)pgd);

	p4d = p4d_offset(pgd, address);
	if (p4d_none(*p4d))
		return -1;
	printk("gpiodriver load \n");
	kfree((void *)address);
	return 0;
}

static void __exit gpiodriver_exit(void)
{
	printk("gpiodriver unload \n");
}

module_init(gpiodriver_init);
module_exit(gpiodriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
