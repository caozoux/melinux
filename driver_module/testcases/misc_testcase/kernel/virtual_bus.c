#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/of.h>
#include <linux/kthread.h>
#include <linux/reboot.h>
#include <linux/uaccess.h>
#include <linux/notifier.h>
#include <linux/interrupt.h>
#include <linux/rcupdate.h>
#include <linux/delay.h>

#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"
#include "mekernel.h"

struct devbusdrv_data {
	struct device dev;
	struct class *metest_class;
 	struct idr metest_idr;
} *dbd_dt;

static void virtual_test_device_release(struct device *dev)
{

}

struct device * virtual_test_get_device(void)
{
	return &dbd_dt->dev;
}

int virme_bus_match(struct device *dev, struct device_driver *drv)
{
	return 0;
}

int virme_bus_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	return 0;
}

struct bus_type virme_bus_type = {
	.name    = "virtual_test_bus",
	.match   = virme_bus_match,
	.uevent = virme_bus_uevent,
};

int virtual_test_bus_init(void)
{
	int res;

	dbd_dt = kzalloc(sizeof(struct devbusdrv_data), GFP_KERNEL);
    res = bus_register(&virme_bus_type);

	idr_init(&dbd_dt->metest_idr);
    dbd_dt->metest_class = class_create(THIS_MODULE, "metest");
	idr_init(&dbd_dt->metest_idr);

	//dbd_dt->dev = device_create(dbd_dt->metest_class, NULL, MKDEV(0, 0), NULL, "test%d", 0);
	device_initialize(&dbd_dt->dev);
	dev_set_name(&dbd_dt->dev, "%s", "vir_dev0");
	dbd_dt->dev.bus = &virme_bus_type;
	dbd_dt->dev.release = virtual_test_device_release;
	device_add(&dbd_dt->dev);
}

int virtual_test_bus_exit(void)
{
	//device_destroy(dbd_dt->metest_class, MKDEV(0,0));
	device_unregister(&dbd_dt->dev);

	class_destroy(dbd_dt->metest_class);
	idr_destroy(&dbd_dt->metest_idr);
    bus_unregister(&virme_bus_type);
    kfree(dbd_dt);
}
