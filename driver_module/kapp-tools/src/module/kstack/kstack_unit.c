#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/bitops.h>

#include <ksysd.h>
#include <kpercpu.h>
#include "ksysdata.h"
#include "ksysd_ioctl.h"
#include "ksioctl/kstack_ioctl.h"
#include "internal.h"

extern void stack_exit(void);
extern int stack_init(void);

int kstack_unit_ioctl_func(unsigned int cmd, unsigned long size, struct ioctl_ksdata *data)
{
	int ret = 0;
	struct kstack_ioctl kioctl;

	DBG("subcmd:%d\n", (int)data->subcmd);
	if (copy_from_user(&kioctl, (char __user *)data->data, sizeof(struct kstack_ioctl))) {
		printk("ioctl data copy err\n");
		ret = -EFAULT;
		goto OUT;
	}

	switch (data->subcmd) {
		case IOCTL_KSTACK_DUMP:
			ret = user_get_ksys_callchain_buffers(kioctl.buf, kioctl.size);
			break;

		case IOCTL_KSTACK_CLEAN:
			clear_ksys_callchain_buffers();
			ret = 0;
			break;
		default:
			break;
	}

OUT:
	return ret;
}

int kstack_unit_init(void)
{
	if (stack_init())
		return -EINVAL;
	ksys_stack_dump(NULL);
	return 0;
}

int kstack_unit_exit(void)
{
	stack_exit();
	return 0;
}

