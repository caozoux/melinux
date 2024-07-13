#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <ksioctl/ksched_ioctl.h>

#include "hotfix_util.h"
#include "ksysdata.h"
#include "ksysd_ioctl.h"
#include "ksched_local.h"

struct proc_dir_entry *ksched_proc;
struct rq __percpu * orig_runqueues;

#if 0
#define PROC_MARCO(__name) \
	static const struct file_operations __name ## _fops = {
		.owner      = THIS_MODULE,
		.open           = __name ## _open,
		.read           = seq_read,
		.llseek         = seq_lseek,
		.release        = signal_release,
	};
#endif


int ksched_unit_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_ksdata *ksdata)
{
	struct ksched_ioctl kioctl;
	int ret;
	void *pbuf ;

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

int ksched_unit_init(void)
{
	LOOKUP_SYMS(runqueues);

	ksched_proc = proc_mkdir("ksched", ksys_proc_root);
	if (!ksched_proc)
		return -EINVAL;

	if (ksched_domain_init())
		return -EINVAL;


	return 0;
}

int ksched_unit_exit(void)
{
	ksched_domain_exit();
	proc_remove(ksched_proc);
	return 0;
}

