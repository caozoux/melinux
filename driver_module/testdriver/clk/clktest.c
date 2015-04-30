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
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/list.h>

#define DT_CLK(dev, con, name)		\
	{				\
		.lk = {			\
			.dev_id = dev,	\
			.con_id = con,	\
		},			\
		.node_name = name,	\
	}

struct testclk_dt_clk {
	struct clk_lookup		lk;
	char				*node_name;
};

static struct testclk_dt_clk test_clks[] = {
	DT_CLK(NULL, "clk_32768_ck", "clk_32768_ck"),
	DT_CLK(NULL, "clk_rc32k_ck", "clk_rc32k_ck"),
	DT_CLK(NULL, "virt_19200000_ck", "virt_19200000_ck"),
	DT_CLK(NULL, "virt_24000000_ck", "virt_24000000_ck"),
	DT_CLK(NULL, "virt_25000000_ck", "virt_25000000_ck"),
	DT_CLK(NULL, "virt_26000000_ck", "virt_26000000_ck"),
	DT_CLK(NULL, "sys_clkin_ck", "sys_clkin_ck"),
	DT_CLK(NULL, "tclkin_ck", "tclkin_ck"),
	DT_CLK(NULL, "dpll_core_ck", "dpll_core_ck"),
	DT_CLK(NULL, "clktest", "clktest"),
	DT_CLK(NULL, "clktest1", "clktest1"),
};

static const struct of_device_id omap_gpio_match[] = {
	{
		.compatible = "zzclktest",
		//.data = &gpiotest_pdata,
	},
	{},
};
MODULE_DEVICE_TABLE(of, omap_gpio_match);

static int meclk_register(struct testclk_dt_clk *oclks)
{
	struct clk *clk, *clk1, *clk2;
	struct device_node *node;
	struct testclk_dt_clk *c;
	struct of_phandle_args clkspec;

	for (c = oclks; c->node_name != NULL; c++) {
		node = of_find_node_by_name(NULL, c->node_name);
		if (!node) {
			continue;
		}

		clkspec.np = node;
		clk = of_clk_get_from_provider(&clkspec);

		if (!IS_ERR(clk)) {
			c->lk.clk = clk;
			clkdev_add(&c->lk);
			printk("zz %s clk\n", c->node_name);
		}
	}

	return 0;
}

static int
clktest_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	dev_info(dev, "drvier probe\n");
	meclk_register(&test_clks);
	return 0;
}

static int
clktest_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	dev_info(dev, "drvier remove\n");
	return 0;
}

static const struct of_device_id ltq_mtd_match[] = {
	{ .compatible = "zzclktest" },
	{},
};

static struct platform_driver gpiotest_drv = {
	.probe =  clktest_probe,
	.remove = clktest_remove,
	.driver = {
		.name = "zzclktest",
		.owner = THIS_MODULE,
		.of_match_table = ltq_mtd_match,
	},
};

static int __init clkdriver_init(void)
{
	platform_driver_register(&gpiotest_drv);
	return 0;
}

static void __exit clkdriver_exit(void)
{
	platform_driver_unregister(&gpiotest_drv);
}

module_init(clkdriver_init);
module_exit(clkdriver_exit);

//module_param(dump, bool, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(dump, "Enable sdio read/write dumps.");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Luciano Coelho <coelho@ti.com>");
MODULE_AUTHOR("Juuso Oikarinen <juuso.oikarinen@nokia.com>");
