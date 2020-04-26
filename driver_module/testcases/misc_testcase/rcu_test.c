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
#include <linux/reboot.h>
#include <linux/uaccess.h>
#include <linux/notifier.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

#include "template_iocmd.h"
#include "misc_ioctl.h"


static unsigned long cnt_test;

static int rcu_test_sync(unsigned long addr, struct ioctl_data *data)
{
	return 0;
}

int rcu_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data)
{

#if 0 
	int ret = -1;
	switch (data->cmdcode) {
		case  IOCTL_TYPE_VMALLOC_MAX:
			vmalloc_max();
			break;
		default:
			goto OUT;
	}
OUT:
	return ret;
#endif

	cnt_test = data->args[0];
	cnt_test++;
	udelay(1);
	cnt_test--;

	data->args[0] = cnt_test  ;

	if (copy_to_user((char __user *) addr, data, sizeof(struct ioctl_data))) {
		printk("copy to user failed\n");
		return -1;
	}
	//printk("zz %s %d %ld\n", __func__, __LINE__, cnt_test);

	return 0;
}
