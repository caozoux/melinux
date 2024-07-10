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
}

int ksched_unit_exit(void)
{
	remove_proc_subtree("ksched", ksys_proc_root);
	ksched_domain_exit();
	return 0;
}

