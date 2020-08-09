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
	struct work_struct wq_sigtestwq_spinlockirq_race;
	struct work_struct wq_sigtestwq_performace_delay;

	struct delayed_work wkq_delay_test;

	wait_queue_head_t wait;
	u8 wq_complete;
	unsigned long performance_delay;
	unsigned long performance_exp;

	int runtime;
} *wkq_dt;

static DEFINE_SPINLOCK(locktest_lock);
static DEFINE_SPINLOCK(locktest_irqlock);

static void wkq_delay_test(struct work_struct *work)
{

}

static void wkq_sig_test(struct work_struct *work)
{
	me_mdelay(1);
}

static void wkq_sig_spinlock_test(struct work_struct *work)
{
	struct wkq_data *wkq_d = container_of(work, struct wkq_data, wq_sigtestwq_spinlock);
	unsigned long after_time = msecs_to_jiffies(wkq_d->runtime) + jiffies;

	while(1) {
		spin_lock(&locktest_lock);
		me_mdelay(1);
		spin_unlock(&locktest_lock);
		if (time_after(jiffies, after_time))
			break;
	}
}

static void wkq_sig_spinlockirq_test(struct work_struct *work)
{
	struct wkq_data *wkq_d = container_of(work, struct wkq_data, wq_sigtestwq_spinlockirq);
	unsigned long after_time = msecs_to_jiffies(wkq_d->runtime) + jiffies;
	unsigned long locktest_irqlock_flags;

	while(1) {
		spin_lock_irqsave(&locktest_lock, locktest_irqlock_flags);
		me_mdelay(1);
		spin_unlock_irqrestore(&locktest_lock, locktest_irqlock_flags);
		if (time_after(jiffies, after_time))
			break;
	}
}

static void wkq_sig_spinlockirq_race_test(struct work_struct *work)
{
	struct wkq_data *wkq_d = container_of(work, struct wkq_data, wq_sigtestwq_spinlockirq);
	unsigned long after_time = msecs_to_jiffies(wkq_d->runtime) + jiffies;
	unsigned long locktest_irqlock_flags;

	spin_lock_irqsave(&locktest_lock, locktest_irqlock_flags);
	while(1) {
		me_mdelay(1);
		if (time_after(jiffies, after_time))
			break;
	}
	spin_unlock_irqrestore(&locktest_lock, locktest_irqlock_flags);
}

//caculte the delay time from queuework to run the workqueue function
static void wkq_sig_performace_delay(struct work_struct *work)
{
	struct wkq_data *wkq_dt = container_of(work, struct wkq_data, wq_sigtestwq_performace_delay);
	unsigned long tsc_val = rdtsc();
	wkq_dt->performance_exp = tsc_val - wkq_dt->performance_delay;
	wkq_dt->wq_complete = 1;
	wake_up(&wkq_dt->wait);
}

int workqueue_unit_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data)
{

	int ret = -1;

	wkq_dt->runtime = data->wq_data.runtime;
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
	
		case  IOCTL_USEWORKQUEUE_PERCPU_SPINLOCKIRQ_RACE:
			queue_work(wkq_dt->workqueue, &wkq_dt->wq_sigtestwq_spinlockirq);
			queue_work(wkq_dt->sigtestworkqueue, &wkq_dt->wq_sigtestwq_spinlockirq);
			DEBUG("workqueue sig spinlock irq\n");
			break;

		case  IOCTL_USEWORKQUEUE_PEFORMANCE_DELAY:
			queue_work(wkq_dt->sigtestworkqueue, &wkq_dt->wq_sigtestwq_performace_delay);
			init_waitqueue_head(&wkq_dt->wait);
#ifdef CONFIG_X86
			wkq_dt->performance_delay = rdtsc();
			wkq_dt->wq_complete = 0;
			if (!wait_event_interruptible_timeout(wkq_dt->wait, wkq_dt->wq_complete, 100*HZ))
				ret = -EBUSY;
			else 
				copy_to_user((char __user *) data->wq_data.workqueue_performance,  &wkq_dt->performance_exp,
						8);
#endif
			DEBUG("workqueue performace test\n");
			break;
		default:
			goto OUT;
	}
OUT:
	return ret;
}

int workqueue_unit_init(void)
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
	INIT_WORK(&wkq_dt->wq_sigtestwq_spinlockirq_race, wkq_sig_spinlockirq_race_test);
	INIT_WORK(&wkq_dt->wq_sigtestwq_performace_delay, wkq_sig_performace_delay);
	INIT_DELAYED_WORK(&wkq_dt->wkq_delay_test, wkq_delay_test);

	return 0;
}

int workqueue_unit_exit(void)
{
	destroy_workqueue(wkq_dt->sigtestworkqueue);
	destroy_workqueue(wkq_dt->workqueue);
	kfree(wkq_dt);
	return 0;
}
