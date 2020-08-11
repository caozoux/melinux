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

#include <asm/stacktrace.h>
#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"
#include "medelay.h"

DEFINE_STATIC_KEY_FALSE(caps_ready);

int statickey_unit_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_data *data)
{
	return 0;
}

int statickey_unit_init(void)
{
	if (static_branch_likely(&caps_ready)) {
		printk("first caps_ready true\n");
	} else {
		printk("first caps_ready false\n");
	}
	static_branch_enable(&caps_ready);
	if (static_branch_likely(&caps_ready)) {
		printk("second caps_ready true\n");
	} else {
		printk("second caps_ready false\n");
	}

	return 0;
}

int statickey_unit_exit(void)
{
	return 0;
}

