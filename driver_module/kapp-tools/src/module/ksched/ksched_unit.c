#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>

#include <ksysd.h>
#include <kpercpu.h>
#include "ksysdata.h"
#include "ksysd_ioctl.h"
#include "ioctl_kprobe.h"
#include "ktrace.h"
#include "ksioctl/ksched_ioctl.h"
#include "ksched.h"

struct rq __percpu *orig_runqueues;

int ksched_unit_ioctl_func(unsigned int cmd, unsigned long size, struct ioctl_ksdata *data)
{
	int ret = 0;
	return ret;
}

static int sysm_init(void)
{
	LOOKUP_SYMS(runqueues);
	return 0;
}

struct proc_dir_entry *ksched_proc;
extern struct proc_dir_entry *ksys_proc_root;

int ksched_unit_init(void)
{
	if (sysm_init())
		return -EINVAL;

	ksched_proc = proc_mkdir("ksched", ksys_proc_root);

	if (ksched_domain_init())
		goto out;

	return 0;
out:
	return -EINVAL;
=======
#include <linux/module.h>
#include <linux/uaccess.h>
#include <ksioctl/ksched_ioctl.h>

#include "hotfix_util.h"
#include "ksysdata.h"
#include "ksysd_ioctl.h"
#include "ksched_local.h"

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

static int jbd2_seq_force_copy_show(struct seq_file *m, void *v)
{

	seq_printf(m, "\n");
	return 0;
}

static int jbd2_seq_info_open(struct inode *inode, struct file *file)
{
	 return single_open(filp, jbd2_seq_force_copy_show, NULL);
}

static struct proc_dir_entry *proc_jbd2_stats;


static const struct file_operations jbd2_seq_info_fops = {
	.owner      = THIS_MODULE,
	.open           = jbd2_seq_info_open,
	.read           = seq_read,
	.llseek         = seq_lseek,
	.release        = single_release,
};

int ksched_unit_init(void)
{
	LOOKUP_SYMS(runqueues);

	journal->j_proc_entry = proc_mkdir(journal->j_devname, proc_jbd2_stats);
	proc_create_data("info", S_IRUGO, journal->j_proc_entry,  &jbd2_seq_info_fops, journal);
	return 0;
}

int ksched_unit_exit(void)
{
	remove_proc_subtree("ksched", ksys_proc_root);
	ksched_domain_exit();
	return 0;
}

