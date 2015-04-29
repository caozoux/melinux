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

struct prv_data {
	int flags;
};
static bool dump;

static LIST_HEAD(omap_gpio_list);

struct gpio_regs {
	u32 irqenable1;
	u32 irqenable2;
	u32 wake_en;
	u32 ctrl;
	u32 oe;
	u32 leveldetect0;
	u32 leveldetect1;
	u32 risingdetect;
	u32 fallingdetect;
	u32 dataout;
	u32 debounce;
	u32 debounce_en;
};

struct gpio_bank {
	struct list_head node;
	void __iomem *base;
	u16 irq;
	struct irq_domain *domain;
	u32 non_wakeup_gpios;
	u32 enabled_non_wakeup_gpios;
	struct gpio_regs context;
	u32 saved_datain;
	u32 level_mask;
	u32 toggle_mask;
	spinlock_t lock;
	struct gpio_chip chip;
	struct clk *dbck;
	u32 mod_usage;
	u32 irq_usage;
	u32 dbck_enable_mask;
	bool dbck_enabled;
	struct device *dev;
	bool is_mpuio;
	bool dbck_flag;

	bool context_valid;
	int stride;
	u32 width;
	int context_loss_count;
	int power_mode;
	bool workaround_enabled;

	void (*set_dataout)(struct gpio_bank *bank, int gpio, int enable);
	int (*get_context_loss_count)(struct device *dev);

	struct omap_gpio_reg_offs *regs;
};

struct gpiotest_platform_data {
	int bank_type;
	int bank_width;		/* GPIO bank width */
	int bank_stride;	/* Only needed for omap1 MPUIO */
	bool dbck_flag;		/* dbck required or not - True for OMAP3&4 */
	bool is_mpuio;		/* whether the bank is of type MPUIO */
	u32 non_wakeup_gpios;
	//struct omap_gpio_reg_offs *regs;
	/* Return context loss count due to PM states changing */
	int (*get_context_loss_count)(struct device *dev);
};

void omap2_gpio_prepare_for_idle(int pwr_mode)
{
}

void omap2_gpio_resume_after_idle(void)
{
}

/* use platform_driver for this. */
static struct platform_driver omap_mpuio_driver = {
	.driver		= {
		.name	= "mpuio",
		//.pm	= &omap_mpuio_dev_pm_ops,
	},
};

static struct platform_device omap_mpuio_device = {
	.name		= "mpuio",
	.id		= -1,
	.dev = {
		.driver = &omap_mpuio_driver.driver,
	}
	/* could list the /proc/iomem resources */
};

static inline void mpuio_init(struct gpio_bank *bank)
{
	platform_set_drvdata(&omap_mpuio_device, bank);

	if (platform_driver_register(&omap_mpuio_driver) == 0)
		(void) platform_device_register(&omap_mpuio_device);
}

static void gpio_irq_shutdown(struct irq_data *data)
{

}

static void gpio_irq_ack(struct irq_data *data)
{

}

static void gpio_irq_mask(struct irq_data *data)
{

}

static void gpio_irq_unmask(struct irq_data *data)
{

}

static int gpio_irq_set_type(struct irq_data *data, unsigned int flow_type)
{
	return 0;
}

static int gpio_irq_set_wake(struct irq_data *data, unsigned int on)
{
	return 0;
}

void gpio_set_dataout_reg(struct gpio_bank *bank, int gpio, int enable)
{

}

void gpio_set_dataout_mask(struct gpio_bank *bank, int gpio, int enable)
{

}

int gpio_get_context_loss_count(struct device *dev)
{
	return 0;
}

static int omap_gpio_request(struct gpio_chip *chip, unsigned offset) 
{
	dev_dbg(chip->dev,"%s  %d\n", __func__, offset);
	return 0;
}

static void omap_gpio_free(struct gpio_chip *chip, unsigned offset)
{
	dev_dbg(chip->dev,"%s  %d\n",__func__, offset);
}

static int  gpio_input(struct gpio_chip *chip, unsigned offset)
{
	dev_dbg(chip->dev,"%s  %d\n",__func__, offset);
	return 0;
}

static int  gpio_get(struct gpio_chip *chip, unsigned offset)
{
	dev_dbg(chip->dev,"%s  %d\n",__func__, offset);
	return 0;
}

static int  gpio_output(struct gpio_chip *chip, unsigned offset, int value)
{
	dev_dbg(chip->dev,"%s %d value:%d\n", __func__, offset, value);
	return 0;
}

static int  gpio_debounce(struct gpio_chip *chip, unsigned offset, unsigned debounce)
{
	dev_dbg(chip->dev,"%s  %d debounce:%d \n", __func__, offset, debounce);
	return 0;
}

static int  gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	dev_dbg(chip->dev," %d \n", offset);
	return 0;
}

static int  omap_gpio_to_irq(struct gpio_chip *chip, unsigned offset)
{
	dev_dbg(chip->dev,"%s  %d\n",__func__, offset);
	dump_stack();
	return 0;
}
/*
 *
 */
static void omap_gpio_chip_init(struct gpio_bank *bank, struct irq_chip *irqc)
{
	static int gpio = 32;

	/*
	 * REVISIT eventually switch from OMAP-specific gpio structs
	 * over to the generic ones
	 */
	bank->chip.request = omap_gpio_request;
	bank->chip.free = omap_gpio_free;
	bank->chip.direction_input = gpio_input;
	bank->chip.get = gpio_get;
	bank->chip.direction_output = gpio_output;
	bank->chip.set_debounce = gpio_debounce;
	bank->chip.set = gpio_set;
	bank->chip.to_irq = omap_gpio_to_irq;

	if (bank->is_mpuio) {
		bank->chip.label = "mpuio";
		if (bank->regs->wkup_en)
			bank->chip.dev = &omap_mpuio_device.dev;
		bank->chip.base = OMAP_MPUIO(0);
	} else {
		bank->chip.label = "gpio";
		bank->chip.base = gpio;
		gpio += bank->width;
	}

	bank->chip.ngpio = bank->width;
	bank->chip.dev = bank->dev;

	if (gpiochip_add(&bank->chip)) {
		dev_info(bank->dev, "add gpio chip: base:%08x label:%s ngpio:%d\n",
				bank->chip.base, bank->chip.label, bank->chip.ngpio);
	}
	else {
		dev_err(bank->dev, "add gpio chip failed\n");
	}
}

static const struct gpiotest_platform_data gpiotest_pdata = {
	//.regs = &omap2_gpio_regs,
	.bank_width = 32,
	.dbck_flag = true,
};

static const struct of_device_id omap_gpio_match[] = {
	{
		.compatible = "zzgpiotest",
		.data = &gpiotest_pdata,
	},
	{},
};
MODULE_DEVICE_TABLE(of, omap_gpio_match);

static int gpio_blanck_init(struct platform_device *pdev)
{
	struct gpio_bank *bank;
	struct irq_chip *irqc;
	struct device *dev = &pdev->dev;
	struct resource *res;
	const struct omap_gpio_platform_data *pdata;
	struct device_node *node = dev->of_node;
	const struct of_device_id *match;

	match = of_match_device(of_match_ptr(omap_gpio_match), dev);
	pdata = match ? match->data : dev_get_platdata(dev);
	if (!pdata) {
		dev_err(dev, "no pdata\n");
		return -EINVAL;
	}

	bank = kzalloc(sizeof(struct gpio_bank), GFP_KERNEL);
	if (!bank) {
		dev_err(dev, "Memory alloc failed\n");
		return -ENOMEM;
	}

	irqc = devm_kzalloc(dev, sizeof(*irqc), GFP_KERNEL);
	if (!irqc)
		return -ENOMEM;

	irqc->irq_shutdown = gpio_irq_shutdown;
	irqc->irq_ack = gpio_irq_ack;
	irqc->irq_mask = gpio_irq_mask;
	irqc->irq_unmask = gpio_irq_unmask;
	irqc->irq_set_type = gpio_irq_set_type;
	irqc->irq_set_wake = gpio_irq_set_wake;
	irqc->name = dev_name(&pdev->dev);

#if 0
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (unlikely(!res)) {
		dev_err(dev, "Invalid IRQ resource\n");
		return -ENODEV;
	}
#else
	res = kzalloc(sizeof(struct resource), GFP_KERNEL);
	res->start= 0x2c200000;
	res->end = res->start+0x10000;
#endif

	bank->irq = res->start;
	bank->dev = dev;
	bank->dbck_flag = pdata->dbck_flag;
	bank->stride = pdata->bank_stride;
	bank->width = pdata->bank_width;
	bank->is_mpuio = pdata->is_mpuio;
	bank->non_wakeup_gpios = pdata->non_wakeup_gpios;
	bank->regs = pdata->regs;

#ifdef CONFIG_OF_GPIO
	bank->chip.of_node = of_node_get(node);
#endif
	if (!node) {
		bank->get_context_loss_count =
			pdata->get_context_loss_count;
	}

	bank->domain = irq_domain_add_linear(node, bank->width,
					     &irq_domain_simple_ops, NULL);

	bank->set_dataout = gpio_set_dataout_mask;

	spin_lock_init(&bank->lock);

	bank->base = devm_ioremap(dev, res->start, resource_size(res));
	if (!bank->base) {
		dev_err(dev, "Could not ioremap\n");
		irq_domain_remove(bank->domain);
		return -ENOMEM;
	}

	pm_runtime_enable(bank->dev);
	pm_runtime_irq_safe(bank->dev);
	pm_runtime_get_sync(bank->dev);

	if (bank->is_mpuio)
		mpuio_init(bank);

	//omap_gpio_mod_init(bank);
	omap_gpio_chip_init(bank, irqc);
	//omap_gpio_show_rev(bank);

	pm_runtime_put(bank->dev);

	list_add_tail(&bank->node, &omap_gpio_list);
	dev_info(dev, "probe.\n");
}

static int
gpiotest_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	dev_info(dev, "drvier probe\n");
	gpio_blanck_init(pdev);
	return 0;
}

static int
gpiotest_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	dev_info(dev, "drvier remove\n");
	return 0;
}

static const struct of_device_id ltq_mtd_match[] = {
	{ .compatible = "zzgpiotest" },
	{},
};

static struct platform_driver gpiotest_drv = {
	.probe =  gpiotest_probe,
	.remove = gpiotest_remove,
	.driver = {
		.name = "zzgpiotest",
		.owner = THIS_MODULE,
		.of_match_table = ltq_mtd_match,
	},
};  

static int __init gpiodriver_init(void)
{
	printk("zz %s \n", __func__);
	platform_driver_register(&gpiotest_drv);
	return 0;
}

static void __exit gpiodriver_exit(void)
{
	printk("zz %s \n", __func__);
	platform_driver_unregister(&gpiotest_drv);
}

module_init(gpiodriver_init);
module_exit(gpiodriver_exit);

module_param(dump, bool, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(dump, "Enable sdio read/write dumps.");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Luciano Coelho <coelho@ti.com>");
MODULE_AUTHOR("Juuso Oikarinen <juuso.oikarinen@nokia.com>");
