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


int devbusdrvtest_unit_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data)
{

	int ret = -1;

	switch (data->cmdcode) {
		case  IOCTL_USERCU_READTEST_START:
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

int devbusdrvtest_unit_init(void)
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
	virtual_test_bus_init();
	return 0;
	
}

int devbusdrvtest_unit_exit(void)
{
	virtual_test_bus_exit();
	return 0;
}


