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

static DEFINE_SPINLOCK(locktest_lock);

int locktest_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_data *data)
{
	int ret = -1;
	switch (data->cmdcode) {
		case  IOCTL_HARDLOCK_LOCK:
			DEBUG("spinlock lock\n");
			//spin_lock_irqsave(&davinci_rtc_lock, flags);
			spin_lock(&locktest_lock);
			//rcu_readlock_test_start();
			break;
		case  IOCTL_HARDLOCK_UNLOCK:
			DEBUG("spinlock unlock\n");
			spin_unlock(&locktest_lock);
			//rcu_readlock_test_stop();
			break;
		case  IOCTL_HARDLOCK_TRYLOCK:
			DEBUG("spinlock trylock\n");
			spin_trylock(&locktest_lock);
			//rcu_readlock_test_stop();
			break;
		default:
			goto OUT;
	}

OUT:
	return 0;
}

int locktest_init(void)
{
}

int locktest_exit(void)
{

}

