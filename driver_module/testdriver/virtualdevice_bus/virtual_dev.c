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
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/kdev_t.h>
#include "virtual_platform.h"
#include "virtual_bus.h"

static DEFINE_IDR(virme_devices);
#define VIRME_MARJOR (219)

extern struct virme_bus_data virme_bus_d;
struct device *alloce_virtualme_dev(void *drvdata, char * name)
{
	int res;
	struct device *dev;
	dev_t dev_mr;
	int minor;
		
	dev->bus = &virme_bus_type;
	dev = kzalloc(sizeof(dev), GFP_KERNEL);
	if (!dev)
		goto out3;
#if 0
	minor = idr_alloc(&virme_devices, NULL, 0, 0, GFP_KERNEL);

	if (minor < 0) {
		pr_err("%s alloc idr failed.\n", name ? name : "");
		goto out2;
	}

	dev_mr = MKDEV(VIRME_MARJOR, minor);
	dev = device_create(NULL, &virme_bus_d.dev,
			MKDEV(VIRME_MARJOR, minor), NULL,
			name);
#else
	dev = device_create(NULL, &virme_bus_d.dev,
			0, NULL,
			name);
#endif

	if (!dev) 
		goto out1;

	return dev;
out1:
out2:
	kfree(dev);
out3:
	pr_err("%s device create failed.\n", name ? name : "");
	return NULL;
};

void free_virtualme_dev(struct device *dev)
{
	device_destroy(NULL,dev->devt);
}
