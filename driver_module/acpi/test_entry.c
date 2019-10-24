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
#include <linux/acpi.h>
#include <linux/acpi_pmtmr.h>
#include <asm/io_apic.h>
#include <asm/apic.h>

static int __init acpi_parse_sbf(struct acpi_table_header *table)
{
	printk("zz %s %d table:%lx\n", __func__, __LINE__, (unsigned long) table);
	return 0;
}

static void testcode(void)
{
	struct acpi_table_header *table = NULL;
	acpi_get_table(ACPI_SIG_BOOT, 0, &table);
	printk("zz %s table:%lx \n",__func__, (unsigned long)table);

	acpi_get_table(ACPI_SIG_FADT, 0, &table);
	printk("zz %s table:%lx \n",__func__, (unsigned long)table);
	acpi_get_table(ACPI_SIG_HPET, 0, &table);
	
	printk("zz %s table:%lx \n",__func__, (unsigned long)table);
}

static int __init gpiodriver_init(void)
{
	testcode();
	printk("gpiodriver load \n");
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
