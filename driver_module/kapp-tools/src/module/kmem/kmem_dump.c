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

static void dump_watermemory(void)
{
	 for_each_populated_zone(zone) {
		 struct zone *zone;
	 }
}

int kmem_dump_func(struct kmem_ioctl *data)
{

	int ret = 0;
	struct kmem_dump dump_data;
	if (copy_from_user(&dump_data, (char __user *)data->data, sizeof(struct kmem_dump))) {
		pr_err("ioctl data copy err\n");
		ret = -EFAULT;
		goto OUT;
	}

	switch (dump_data.dumpcmd) {
		case IOCTL_USEKMEM_DUMP_MEMORYWARTER:
			dump_watermemory();
			break;
		case IOCTL_USEKMEM_DUMP_EACH_CSS:
			break;

		default:
			break;
	}

	return 0;
OUT:
	return ret;
}

