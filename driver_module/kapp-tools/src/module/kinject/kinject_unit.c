#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <ksioctl/kinject_ioctl.h>

#include "ksysdata.h"
#include "ksysd_ioctl.h"
#include "kinject_local.h"

struct timer_list recover_timer;
static int kinject_warn(void)
{
	WARN_ON(1);
	return 0;
}

static int kinject_list_corrupt(void)
{
	struct list_head head;
	struct list_head corruption_prev;
	struct list_head corruption_next;
	struct list_head test1;
	struct list_head test2;

	INIT_LIST_HEAD(&head);
	INIT_LIST_HEAD(&corruption_prev);
	INIT_LIST_HEAD(&corruption_next);
	INIT_LIST_HEAD(&test1);
	INIT_LIST_HEAD(&test2);

	//double add wanring
	list_add(&head, &head);
	corruption_prev.next = &test1;
	list_add(&test2, &corruption_prev);

	//double add next wanring
	corruption_next.next= &test1;
	list_add_tail(&test2, &corruption_next);
	return 0;
}

static struct mutex hung_lock;

static void inject_time_func(struct timer_list *timer)
{
	if (!mutex_trylock(&hung_lock))
    	mutex_unlock(&hung_lock);

}

static int kinject_softlockup(void)
{
	int i;
	for( i=0; i < 300000; i++) {
		udelay(100);
	}
	return 0;
}

static int kinject_rcu_stall(void)
{
	int i;
	for( i=0; i < 2000000; i++) {
		udelay(100);
	}
	return 0;
}

static int kinject_task_hang(void)
{
	timer_setup(&recover_timer, inject_time_func, 0);
	recover_timer.expires = jiffies + 150000*HZ;

    mutex_init(&hung_lock);
    mutex_lock(&hung_lock);
    mutex_lock(&hung_lock);

	return 0;
}

int kinject_unit_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_ksdata *data)
{
	struct kinject_ioctl kioctl;
	int ret;

	if (copy_from_user(&kioctl, (char __user *)data->data, sizeof(struct kinject_ioctl))) {
		pr_err("ioctl data copy err\n");
		ret = -EFAULT;
		goto OUT;
	}

	switch (data->subcmd) {
		//case IOCTL_USEKINJECT_TEST:
			//kinject_test(&kioctl);
		//kinject_test_statickey(kioctl.enable);
		case IOCTL_USEKINJECT_HRTIMER:
			return kinject_timer_func(data->subcmd, &kioctl);

		case IOCTL_INJECT_SLUB_CTRL:
		case IOCTL_INJECT_SLUB_R_OVERWRITE:
		case IOCTL_INJECT_SLUB_L_OVERWRITE:
		case IOCTL_INJECT_SLUB_DOUBLE_FREE:
			return kinject_slub_func(data->subcmd, &kioctl);
		case IOCTL_INJECT_RWSEM_WRITEDOWN:
		case IOCTL_INJECT_RWSEM_WRITEUP:
		case IOCTL_INJECT_RWSEM_READDOWN:
		case IOCTL_INJECT_RWSEM_READUP:
			kinject_rwsem_func(data->subcmd, &kioctl);
			break;
		case IOCTL_INJECT_STACK_OVERWRITE:
			return kinject_stack_segmet_func(data->subcmd, &kioctl);
			break;

		case IOCTL_INJECT_SPINLOCK_DEPLOCK:
		case IOCTL_INJECT_IRQSPINLOCK_DEPLOCK:
		case IOCTL_INJECT_MUTEXT_LOCK:
		case IOCTL_INJECT_MUTEXT_UNLOCK:
		case IOCTL_INJECT_MUTEXT_DEALY:
			kinject_lock_func(data->subcmd, &kioctl);
			break;

		case IOCTL_INJECT_SOFTWATCHDOG_TIMEOUT:
			kinject_softlockup();
			break;
		case IOCTL_INJECT_RUC_HUNG:
			kinject_rcu_stall();
			break;
		case IOCTL_INJECT_TASK_HUNG:
			kinject_task_hang();
			break;
		case IOCTL_INJECT_WARN:
			kinject_warn();
			break;
		case IOCTL_INJECT_LIST_CORRUPT:
			kinject_list_corrupt();
			break;

		default:
			break;
	}

	return 0;
OUT:
	return ret;
}

int kinject_unit_init(void)
{
	kinject_timer_init();
	kinject_slub_init();
	kinject_rwsem_init();
	kinject_kthread_int();
	return 0;
}

int kinject_unit_exit(void)
{
	kinject_timer_remove();
	kinject_slub_remove();
	kinject_rwsem_remove();
	kinject_kthread_remove();
	return 0;
}

