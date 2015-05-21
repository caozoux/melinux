#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/pm_runtime.h>
#include <linux/pm.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/irqdomain.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/platform_data/gpio-omap.h>


static int test_nand_probe(struct platform_device *pdev)
{
	return 0;
}

static int test_nand_remove(struct platform_device *pdev)
{

	mtd_device_parse_register(mtd, NULL, &ppdata, pdata->parts,
				  pdata->nr_parts);
	nand_release(mtd);
	return 0;
}

static int __init gpiodriver_init(void)
{
	printk("zz %s \n", __func__);
	return 0;
}

static void __exit gpiodriver_exit(void)
{
	printk("zz %s \n", __func__);
}

module_init(gpiodriver_init);
module_exit(gpiodriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Luciano Coelho <coelho@ti.com>");
MODULE_AUTHOR("Juuso Oikarinen <juuso.oikarinen@nokia.com>");
