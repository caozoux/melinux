#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/kprobes.h>

#include "ksioctl/kprobe_ioctl.h"
#include "ksysdata.h"
#include "ksysd_ioctl.h"


static int __kprobes kprobe_prehandler(struct kprobe *p, struct pt_regs *regs);

static int kretprobe_handler(struct kretprobe_instance *ri, struct pt_regs *regs);

static int __kprobes kprobe_prehandler(struct kprobe *p, struct pt_regs *regs)
{
	return 0;
}

static int kretprobe_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	return 0;
}

/*
static int kretprobe_entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	return 0;
}
*/

struct kprobe kp1 = {
    .pre_handler = kprobe_prehandler,
    //.post_handler = kprobe_post_handler,
};

struct kretprobe kretp_st = {
	.handler    = kretprobe_handler,
	//.entry_handler  =  kretprobe_entry_handler,
};


int kprobe_unit_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_ksdata *data)
{
	int ret = 0;
	struct kprobe_ioctl kioctl;

	DBG("subcmd:%d\n", (int)data->subcmd);
	if (copy_from_user(&kioctl, (char __user *)data->data, sizeof(struct kprobe_ioctl))) {
		printk("ioctl data copy err\n");
		ret = -EFAULT;
		goto OUT;
	}

	switch (data->subcmd) {
		case IOCTL_KRPOBE_FUNC_DUMP_EN:
			break;
		case IOCTL_KRPOBE_FUNC_DUMP_DIS:
			break;
		case IOCTL_KRPOBE_FUNC_DUMP_CLEAN:
			break;
		default:
			break;
	}

OUT:
	return ret;
}

int kprobe_unit_init(void)
{
	return 0;
}

int kprobe_unit_exit(void)
{
	return 0;
}

