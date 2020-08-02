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
#ifndef CONFIG_ARM64
#include <asm/cpu_entry_area.h>
#include <asm/unwind.h> 
#endif

#include <asm/stacktrace.h>
#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"

struct atomic_test_data {
	struct workqueue_struct *sigtestworkqueue;
	struct workqueue_struct *workqueue;
	struct work_struct wq_sigtestwq;
	struct delayed_work wkq_delay_test;;
} *atm_dt;

static void atm_sig_test(struct work_struct *work)
{
		
}

int atomic_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_data *data)
{
	int ret = -1;
	switch (data->cmdcode) {
		case  IOCTL_USEATOMIC_PERFORMANCE:
			queue_work(atm_dt->sigtestworkqueue, &atm_dt->wq_sigtestwq);
			DEBUG("atomic test\n");
			break;
		default:
			goto OUT;
	}

OUT:
	return 0;
}

int atomic_init(void)
{
	atm_dt = kzalloc(sizeof(struct atomic_test_data), GFP_KERNEL);

	if (!atm_dt)
		return -EINVAL;

	atm_dt->sigtestworkqueue = create_singlethread_workqueue("atomicworkqt");
	if (!atm_dt->sigtestworkqueue) {
		kfree(atm_dt);
		return -EINVAL;
	}

	INIT_WORK(&atm_dt->wq_sigtestwq, atm_sig_test);
	return 0;
}

int atomic_exit(void)
{
	destroy_workqueue(atm_dt->sigtestworkqueue);
	kfree(atm_dt);
}

