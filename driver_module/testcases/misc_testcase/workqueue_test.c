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
#include "medelay.h"

struct wkq_data {
	struct workqueue_struct *sigtestworkqueue;
	struct workqueue_struct *workqueue;
	struct work_struct wq_sigtestwq;
	struct work_struct wq_sigtestwq_spinlock;
	struct work_struct wq_sigtestwq_spinlockirq;
	struct delayed_work wkq_delay_test;;
} *wkq_dt;

static DEFINE_SPINLOCK(locktest_lock);
static DEFINE_SPINLOCK(locktest_irqlock);
static unsigned long locktest_irqlock_flags;

static void wkq_delay_test(struct work_struct *work)
{

}

static void wkq_sig_test(struct work_struct *work)
{
	me_mdelay(20000);
}

static void wkq_sig_spinlock_test(struct work_struct *work)
{
	spin_lock(&locktest_lock);
	me_mdelay(20000);
	spin_unlock(&locktest_lock);
}

static void wkq_sig_spinlockirq_test(struct work_struct *work)
{
	spin_lock_irqsave(&locktest_lock, locktest_irqlock_flags);
	me_mdelay(20000);
	spin_unlock_irqrestore(&locktest_lock, locktest_irqlock_flags);
}

int workqueue_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data)
{

	int ret = -1;

	switch (data->cmdcode) {
		case  IOCTL_USEWORKQUEUE_SIG :
			queue_work(wkq_dt->sigtestworkqueue, &wkq_dt->wq_sigtestwq);
			DEBUG("workqueue sig\n");
			break;
		case  IOCTL_USEWORKQUEUE_SIG_SPINLOCK:
			queue_work(wkq_dt->sigtestworkqueue, &wkq_dt->wq_sigtestwq_spinlock);
			DEBUG("workqueue sig spinlock\n");
			break;
		case  IOCTL_USEWORKQUEUE_SIG_SPINLOCKIRQ:
			queue_work(wkq_dt->sigtestworkqueue, &wkq_dt->wq_sigtestwq_spinlockirq);
			DEBUG("workqueue sig spinlock irq\n");
			break;
		default:
			goto OUT;
	}
OUT:
	return ret;

#if 0
	data->args[0] = cnt_test  ;

	if (copy_to_user((char __user *) addr, data, sizeof(struct ioctl_data))) {
		printk("copy to user failed\n");
		return -1;
	}
#endif

	return 0;
}

int workqueue_test_init(void)
{

	wkq_dt = kzalloc(sizeof(struct wkq_data), GFP_KERNEL);

	if (!wkq_dt)
		return -EINVAL;

	wkq_dt->sigtestworkqueue = create_singlethread_workqueue("sigworkqt");
	if (!wkq_dt->sigtestworkqueue)
		return -EINVAL;

	wkq_dt->workqueue = create_workqueue("workqt");
	if (!wkq_dt->workqueue)
		return -EINVAL;

	//num_online_cpus

	INIT_WORK(&wkq_dt->wq_sigtestwq, wkq_sig_test);
	INIT_WORK(&wkq_dt->wq_sigtestwq_spinlock, wkq_sig_spinlock_test);
	INIT_WORK(&wkq_dt->wq_sigtestwq_spinlockirq, wkq_sig_spinlockirq_test);
	INIT_DELAYED_WORK(&wkq_dt->wkq_delay_test, wkq_delay_test);

	return 0;
}

void workqueue_test_exit(void)
{
	destroy_workqueue(wkq_dt->sigtestworkqueue);
	destroy_workqueue(wkq_dt->workqueue);
	kfree(wkq_dt);
}
