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
#include <linux/reboot.h>
#include <linux/uaccess.h>
#include <linux/notifier.h>
#include <linux/interrupt.h>

#include "template_iocmd.h"
#include "misc_ioctl.h"


#define VMALLOC_MAX (1024*1024*1024)

static void vmalloc_max(void)
{
	void *addr;

	addr = vmalloc(VMALLOC_MAX);

	if (!addr) {
		printk("vmalloc %llx falied\n", (u64)VMALLOC_MAX);
		return;
	} else {
		printk("vmalloc %llx success\n", (u64)VMALLOC_MAX);
	}

	vfree(addr);

}

int mem_ioctl_func(unsigned int  cmd, struct ioctl_data *data)
{
	int ret = -1;
	switch (data->cmdcode) {
		case  IOCTL_TYPE_VMALLOC_MAX:
			vmalloc_max();
			break;
		default:
			goto OUT;
	}
OUT:
	return ret;
}
