#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/notifier.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/timekeeper_internal.h>
#include <kernel/sched/sched.h>
#include <asm/stacktrace.h>
#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"
#include "medelay.h"


int inject_unit_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_data *data)
{
	void *inject_pointer;
	DEBUG("%s %d\n", __func__, data->cmdcode)
	switch (data->cmdcode) {
		case IOCTL_INJECT_NULL:
			printk("zz %s %d \n", __func__, __LINE__);
			DEBUG("inject NULL pointer");
			*(int *)inject_pointer = 0;
			break;
		case IOCTL_INJECT_WRITE_PROTECT:
			DEBUG("inject write protect");
			inject_pointer = inject_unit_ioctl_func;
			*(int *)inject_pointer = 0;
			break;
		case IOCTL_INJECT_MUTET_DEPLOCK:
			break;
		case IOCTL_INJECT_SPINLOCK_DEPLOCK:
			break;
		case IOCTL_INJECT_IRQSPINLOCK_DEPLOCK:
			break;
		case IOCTL_INJECT_RUC_HANG:
			break;
		case IOCTL_INJECT_SOFTWATCHDOG_TIMEOUT:
			break;
		default:
			break;
	}

	return 0;
}

int inject_unit_init(void)
{
	return 0;
}

int inject_unit_exit(void)
{
	return 0;
}

