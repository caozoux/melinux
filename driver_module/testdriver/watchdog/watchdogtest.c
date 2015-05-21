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

int start(struct watchdog_device *dev)
{
	return 0;
}
int stop(struct watchdog_device *dev)
{
	return 0;
}

int ping(struct watchdog_device *dev)
{
	return 0;
}

int set_timeout(struct watchdog_device *dev, unsigned int time)
{
	return 0;
}

static const struct watchdog_ops omap_wdt_ops = {
	.owner		= THIS_MODULE,
	.start		= omap_wdt_start,
	.stop		= omap_wdt_stop,
	.ping		= omap_wdt_ping,
	.set_timeout	= omap_wdt_set_timeout,
};

int watchdogvir_init(void)
{
	struct watchdog_device *wdtvir;


struct device *device_create(struct class *cls, struct device *parent,
			     dev_t devt, void *drvdata,
			     const char *fmt, ...);
	device_create(NULL,NULL,

	wdtvir= 

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
