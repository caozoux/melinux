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
#include <linux/rwsem.h>

#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"
#include "mekernel.h"

static DEFINE_SPINLOCK(locktest_lock);
static unsigned long locktest_irqlock_flags;
struct semaphore semaphore_cnt;
struct rw_semaphore semaphore_rw;
struct track track;

int locktest_unit_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_data *data)
{
	switch (data->cmdcode) {
		case IOCTL_HARDLOCK_LOCK:
			DEBUG("spinlock lock\n");
			//spin_lock_irqsave(&davinci_rtc_lock, flags);
			spin_lock(&locktest_lock);
			break;
		case IOCTL_HARDLOCK_UNLOCK:
			DEBUG("spinlock unlock\n");
			spin_unlock(&locktest_lock);
			break;
		case IOCTL_HARDLOCK_TRYLOCK:
			DEBUG("spinlock trylock\n");
			spin_trylock(&locktest_lock);
			break;
		case IOCTL_HARDLOCK_IRQLOCK:
			DEBUG("spinlock irq lock\n");
			//spin_lock_irqsave(&davinci_rtc_lock, flags);
			spin_lock_irqsave(&locktest_lock, locktest_irqlock_flags);
			break;
		case IOCTL_HARDLOCK_IRQUNLOCK:
			DEBUG("spinlock irq unlock\n");
			spin_unlock_irqrestore(&locktest_lock, locktest_irqlock_flags);
			break;
		case IOCTL_HARDLOCK_IRQTRYLOCK:
			DEBUG("spinlock irq trylock\n");
			break;
		case IOCTL_SEMAPHORE_DOWN:
			DEBUG("semaphore down\n");
			down(&semaphore_cnt);
			break;
		case IOCTL_SEMAPHORE_UP:
			DEBUG("semaphore up\n");
			up(&semaphore_cnt);
			break;
		case IOCTL_SEMAPHORE_READ_DOWN:
			DEBUG("semaphore read down\n");
			down_read(&semaphore_rw);
			break;
		case IOCTL_SEMAPHORE_READ_UP:
			DEBUG("semaphore read up\n");
			up_read(&semaphore_rw);
			break;
		case IOCTL_SEMAPHORE_WRITE_DOWN:
			DEBUG("semaphore write down\n");
			down_write(&semaphore_rw);
			break;
		case IOCTL_SEMAPHORE_WRITE_UP:
			DEBUG("semaphore write up\n");
			up_write(&semaphore_rw);
			break;
		default:
			goto OUT;
	}

	printk("semaphore_cnt %d\n", semaphore_cnt.count);
	printk("semaphore_rw: %ld\n", atomic_long_read(&semaphore_rw.count));
OUT:
	return 0;
}

int locktest_unit_init(void)
{
	sema_init(&semaphore_cnt, 1);
	init_rwsem(&semaphore_rw);
	return 0;
}

int locktest_unit_exit(void)
{
	return 0;
}

