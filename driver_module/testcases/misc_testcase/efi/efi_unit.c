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
#include <linux/efi.h>

#include <asm/stacktrace.h>
#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"
#include "medelay.h"

struct efi *orig_efi;

int efi_unit_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_data *data)
{

	return 0;
}

int efi_unit_init(void)
{
	LOOKUP_SYMS(efi);
	printk("zjz %s efi:%lx %lx\n",__func__, (unsigned long)orig_efi, orig_efi->reset_system);
	//orig_efi->reset_system( EFI_RESET_COLD, EFI_SUCCESS, 0, NULL); 
	//orig_efi->reset_system( EFI_RESET_WARM, EFI_SUCCESS, 0, NULL); 
	return 0;
}

int efi_unit_exit(void)
{
	return 0;
}

