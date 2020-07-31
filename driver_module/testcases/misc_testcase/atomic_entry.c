#include <linux/kallsyms.h>
#include <linux/kprobes.h>
#include <linux/uaccess.h>
#include <linux/utsname.h>
#include <linux/hardirq.h>
#include <linux/kdebug.h>
#include <linux/module.h>
#include <linux/ptrace.h>
#include <linux/sched/debug.h>
#include <linux/sched/task_stack.h>
#include <linux/ftrace.h>
#include <linux/kexec.h>
#include <linux/bug.h>
#include <linux/nmi.h>
#include <linux/sysfs.h>
#include <linux/kasan.h>

#ifndef CONFIG_ARM64
#include <asm/cpu_entry_area.h>
#include <asm/unwind.h> 
#endif

#include <asm/stacktrace.h>
#include "template_iocmd.h"

struct atomic_test_data {
	struct workqueue_struct *sigtestworkqueue;
	struct workqueue_struct *workqueue;
	struct work_struct wq_sigtestwq;
	struct delayed_work wkq_delay_test;;
} *atm_dt;

static void wkq_sig_test(struct work_struct *work)
{
		
}

int atomic_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_data *data)
{
	int ret = -1;
	switch (data->cmdcode) {
		case  IOCTL_USEATOMIC_PERFORMANCE:
			queue_work(atm_dt->sigtestworkqueue, &wkq_dt->wq_sigtestwq);
			DEBUG("atomic test\n");
			break;
		default:
			goto OUT;
	}

OUT:
	return 0;
	return 0;
}

int atomic_init(void)
{
	atm_dt = kzalloc(sizeof(struct wkq_data), GFP_KERNEL);

	if (!wkq_dt)
		return -EINVAL;

	atm_dt->sigtestworkqueue = create_singlethread_workqueue("atomicworkqt");
	if (!wkq_dt->sigtestworkqueue) {
		kfree(atm_dt);
		return -EINVAL;
	}

	INIT_WORK(&atm_dt->wq_sigtestwq, wkq_sig_test);
	return 0;
}

int atomic_exit(void)
{
	destroy_workqueue(wkq_dt->sigtestworkqueue);
	kfree(atm_dt);
}

