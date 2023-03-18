#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <ksioctl/kmem_ioctl.h>

#include "ksysdata.h"
#include "ksysd_ioctl.h"
#include "kmem_local.h"

int kmem_unit_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_ksdata *ksdata)
{
	struct kmem_ioctl kioctl;
	int ret;

	if (copy_from_user(&kioctl, (char __user *)ksdata->data, ksdata->len)) {
		pr_err("ioctl data copy err\n");
		ret = -EFAULT;
		goto OUT;
	}

	switch (ksdata->subcmd) {
		case IOCTL_USEKMEM_DUMP:
			ret = kmem_dump_func(&kioctl);
			break;

		case IOCTL_USEKMEM_CGROUP_SCANKMEM:
			dump_cgroup_kmem_info(kioctl.pid);
			break;

		default:
			break;
	}

	return 0;

OUT:
	return ret;
}

int kmem_unit_init(void)
{
	//LOOKUP_SYMS(get_slabinfo);

	kmem_cgroup_init();
	return 0;
}

int kmem_unit_exit(void)
{
	kmem_cgroup_exit();
	return 0;
}

