#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <ksioctl/kinject_ioctl.h>

#include "ksysdata.h"
#include "ksysd_ioctl.h"
#include "kinject_local.h"

int kinject_unit_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_ksdata *data)
{
	struct kinject_ioctl kioctl;
	int ret;

	if (copy_from_user(&kioctl, (char __user *)data->data, sizeof(struct kinject_ioctl))) {
		pr_err("ioctl data copy err\n");
		ret = -EFAULT;
		goto OUT;
	}

	switch (data->subcmd) {
		//case IOCTL_USEKINJECT_TEST:
			//kinject_test(&kioctl);
		//kinject_test_statickey(kioctl.enable);
		case IOCTL_USEKINJECT_HRTIMER:
			return kinject_timer_func(data->subcmd, &kioctl);

		case IOCTL_INJECT_SLUB_CTRL:
		case IOCTL_INJECT_SLUB_OVERWRITE:
			return kinject_slub_func(data->subcmd, &kioctl);
		case IOCTL_INJECT_RWSEM_WRITEDOWN:
		case IOCTL_INJECT_RWSEM_WRITEUP:
		case IOCTL_INJECT_RWSEM_READDOWN:
		case IOCTL_INJECT_RWSEM_READUP:
			kinject_rwsem_func(data->subcmd, &kioctl);
			break;
		case IOCTL_INJECT_STACK_OVERWRITE:
			return kinject_stack_segmet_func(data->subcmd, &kioctl);
			break;

		default:
			break;
	}

	return 0;
OUT:
	return ret;
}

int kinject_unit_init(void)
{
	kinject_timer_init();
	kinject_slub_init();
	kinject_rwsem_init();
	return 0;
}

int kinject_unit_exit(void)
{
	kinject_timer_remove();
	kinject_slub_remove();
	kinject_rwsem_remove();
	return 0;
}

