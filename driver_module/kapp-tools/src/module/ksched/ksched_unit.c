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

struct proc_dir_entry *root_dir;
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

static int signal_show(struct seq_file *m, void *v)
{
	seq_printf(m, "zz\n");
	return 0;
}

static int info_open(struct inode *inode, struct file *file)
{
	 return single_open(file, signal_show, NULL);
}

static ssize_t info_proc_write(struct file *file, const char __user *buffer,
		size_t count, loff_t *pos)
{
	return count;
}


static const struct file_operations info_fops = {
	.owner      = THIS_MODULE,
	.open           = info_open,
	.read           = seq_read,
	.llseek         = seq_lseek,
	.release        = single_release,
	.write      = info_proc_write,
};

int ksched_unit_init(void)
{
	LOOKUP_SYMS(runqueues);

	if (ksched_domain_init())
		return -EINVAL;

	root_dir = proc_mkdir("ksched", NULL);
	if (!root_dir)
		return -EINVAL;

	proc_create_data("info", S_IRUGO, root_dir,  &info_fops, NULL);
	return 0;
}

int ksched_unit_exit(void)
{
	ksched_domain_exit();
	proc_remove(root_dir);
	return 0;
}

