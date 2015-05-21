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
#include "virtual_platform.h"
#include "virtual_bus.h"

int virme_bus_match(struct device *dev, struct device_driver *drv)
{
	return 0;
}

int virme_bus_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	return 0;
}

int virme_bus_probe(struct device *dev)
{
	return 0;
}
int virme_bus_remove(struct device *dev)
{
	return 0;
}

void virme_bus_shutdown(struct device *dev)
{
}

int virme_bus_online(struct device *dev)
{
	return 0;
}

int virme_bus_offline(struct device *dev)
{
	return 0;
}

int virme_bus_suspend(struct device *dev, pm_message_t state)
{
	return 0;
}

int virme_bus_resume(struct device *dev)
{
	return 0;
}

struct bus_type virme_bus_type = {
	.name    = "virme_bus",
	.match   = virme_bus_match,
	.uevent = of_device_uevent_modalias,
	.probe   = virme_bus_probe,
	.remove  = virme_bus_remove,
	.shutdown = virme_bus_shutdown,
	.suspend = virme_bus_suspend,
	.resume  = virme_bus_resume,
	//.dev_attrs = virme_dev_attrs,
};

struct virme_bus_data virme_bus_d;
static int virme_bus_driver_init(void)
{
	int res;
	struct device *dev;
    res = bus_register(&virme_bus_type);

	if (!res) {
		printk("bus register failed\n");
		return -1;
	}

	dev = kzalloc(sizeof(dev), GFP_KERNEL);
	if (!dev)
		goto out1;

	res = device_register(dev);
	if (!dev)
		goto out2;

out2:
	kfree(dev);
out1:
    bus_unregister(&virme_bus_type);

}

static int __init vir_init(void)
{
	printk("zz %s \n", __func__);
    return bus_register(&virme_bus_type);
}

static void __exit vir_exit(void)
{
	printk("zz %s \n", __func__);
    bus_unregister(&virme_bus_type);
}

module_init(vir_init);
module_exit(vir_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Luciano Coelho <coelho@ti.com>");
MODULE_AUTHOR("Juuso Oikarinen <juuso.oikarinen@nokia.com>");
