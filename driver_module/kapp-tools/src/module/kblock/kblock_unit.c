#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/netdevice.h>
#include <ksioctl/kblock_ioctl.h>

#include "hotfix_util.h"
#include "ksysdata.h"
#include "ksysd_ioctl.h"
#include "kblock_local.h"

int kblock_unit_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_ksdata *ksdata)
{
	struct kblock_ioctl kioctl;
	int ret;

	if (copy_from_user(&kioctl, (char __user *)ksdata->data, ksdata->len)) {
		pr_err("ioctl data copy err\n");
		ret = -EFAULT;
		goto OUT;
	}

	switch (ksdata->subcmd) {
		default:
			break;
	}

	return 0;
OUT:
	return ret;
}

static int kblock_unit_sym_init(void)
{
	return 0;
}

int kblock_unit_init(void)
{
	if (kblock_unit_sym_init())	
		return -EINVAL;

	//kblock_trace_init();
	return 0;
}

int kblock_unit_exit(void)
{
	//kblock_trace_exit();
	return 0;
}

