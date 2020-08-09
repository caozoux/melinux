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

struct devbusdrv_data {
	struct device *dev;
	struct class *metest_class;
 	struct idr metest_idr;
} *dbd_dt;

int virme_bus_match(struct device *dev, struct device_driver *drv)
{
	return 0;
}

int virme_bus_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	return 0;
}

struct bus_type virme_bus_type = {
	.name    = "virme_bus",
	.match   = virme_bus_match,
	.uevent = virme_bus_uevent,
};

int devbusdrv_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data)
{

	int ret = -1;

	switch (data->cmdcode) {
		case  IOCTL_USERCU_READTEST_START:
#if 0
			device_initialize(&dbd_dt->dev);
		    dev_set_name(ctrl->device, "nvme%d", ctrl->instance);
			DEBUG("rcu_readlock_test_start\n")
			rcu_readlock_test_start();
			dev = device_create(NULL, &virme_bus_d.dev,
			0, NULL,
			"testcase_dev");
#else

#endif
			break;
		case  IOCTL_USERCU_READTEST_END:
			DEBUG("rcu_readlock_test_stop\n")
			//rcu_readlock_test_stop();
			break;
		default:
			goto OUT;
	}

OUT:
	return ret;

#if 0
	cdev_init(&ctrl->cdev, &nvme_dev_fops);
dev_set_name(ctrl->device, "nvme%d", ctrl->instance);
device_initialize(&ctrl->ctrl_device);
dev_set_name(ctrl->device, "nvme%d", ctrl->instance);
#endif
}

int devbusdrvtest_init(void)
{
#if 0
static struct idr dca_idr;
	92     idr_init(&dca_idr);
	ret = idr_alloc(&dca_idr, dca, 0, 0, GFP_NOWAIT);
	struct 
dca_class = class_create(THIS_MODULE, "dca");
	 96     if (IS_ERR(dca_class)) {
 97         idr_destroy(&dca_idr);
 98         return PTR_ERR(dca_class);
 99     }
	dev = device_create(NULL, &virme_bus_d.dev,
	int res;

	device_destroy(dca_class, MKDEV(0, slot + 1));
#endif
	int res;

	dbd_dt = kzalloc(sizeof(struct devbusdrv_data), GFP_KERNEL);
    res = bus_register(&virme_bus_type);

	idr_init(&dbd_dt->metest_idr);
    dbd_dt->metest_class = class_create(THIS_MODULE, "metest");
	idr_init(&dbd_dt->metest_idr);

	dbd_dt->dev = device_create(dbd_dt->metest_class, NULL, MKDEV(0, 0), NULL, "test%d", 0);
	return 0;
	
}

int devbusdrvtest_exit(void)
{
	//device_destroy(dbd_dt->dev, )

	if (dbd_dt->dev)
		device_destroy(dbd_dt->metest_class, MKDEV(0,0));

	class_destroy(dbd_dt->metest_class);
	idr_destroy(&dbd_dt->metest_idr);
    bus_unregister(&virme_bus_type);
    kfree(dbd_dt);
}



