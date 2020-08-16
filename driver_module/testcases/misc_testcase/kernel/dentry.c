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
#include <linux/kthread.h>
#include <linux/uaccess.h>
#include <linux/notifier.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/dcache.h>
#include <linux/fdtable.h>

#include <asm/stacktrace.h>
#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"
#include "medelay.h"
#include "mekernel.h"

void medentry_dump_full_patch(struct dentry *dentry)
{
	struct dentry *parent;

	parent = dentry;
	while (!IS_ROOT(parent)) {
		parent = parent->d_parent;
		printk("%s\n", parent->d_iname);
	}
}

