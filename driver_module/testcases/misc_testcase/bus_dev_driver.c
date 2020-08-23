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

#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"


int bdr_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data)
{

	int ret = -1;
	switch (data->cmdcode) {
		case  IOCTL_USERCU_READTEST_START:
			DEBUG("rcu_readlock_test_start\n")
			rcu_readlock_test_start();
			break;
		case  IOCTL_USERCU_READTEST_END:
			DEBUG("rcu_readlock_test_stop\n")
			//rcu_readlock_test_stop();
			break;
		default:
			goto OUT;
	}
OUT:
	return ret;


	return 0;
}

int bdr_init(void)
{
	return 0;
}

