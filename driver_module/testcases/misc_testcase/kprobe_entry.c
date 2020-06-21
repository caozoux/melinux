#include <linux/init.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/kprobes.h>
#include <linux/ftrace.h>
#include <linux/skbuff.h>
#include <linux/icmp.h>
#include <linux/ip.h>
#include <linux/mm.h>

#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"

struct kprobe_misc_data {
	struct kprobe kp1;
	struct completion kprobe_compl;
	wait_queue_head_t wait;
	u8 kprobe_complete;
	char *dump_buf;
};

static int kprobe_handler(struct kprobe *p, struct pt_regs *regs);
static int kprobe_post_handler(struct kprobe *p, struct pt_regs *regs, unsigned long flags);
static void (*orig_dump_stack_print_info)(const char *log_lvl);

static int kprobe_post_handler(struct kprobe *p, struct pt_regs *regs, unsigned long flags)
{
	return 0;
}

static int kprobe_handler(struct kprobe *p, struct pt_regs *regs)
{
	struct kprobe_misc_data *data = container_of(p, struct kprobe_misc_data, kp1);

	if (!data) {
		WARN("kprobe data is NULL\n");
		goto out;
	}

	misc_show_stack(data->dump_buf, NULL, NULL);

out:
	p->flags |= KPROBE_FLAG_DISABLED;
	data->kprobe_complete = 1;
	if (data)
		wake_up(&data->wait);

	return 0;
}

static int kprobe_func_dump(char *dump_buf, char *name)
{
	int ret;
	struct kprobe_misc_data data;

	if (!name) {
		printk("no symbole specified\n");
		return -EINVAL;
	}

	memset(&data, 0, sizeof(struct kprobe_misc_data));
	init_completion(&data.kprobe_compl);
	init_waitqueue_head(&data.wait);
	data.kp1.symbol_name = name;
	data.kp1.pre_handler = kprobe_handler;

	data.dump_buf = dump_buf;

	if (!data.dump_buf)
		return -ENOMEM;

    ret = register_kprobe(&data.kp1);
	if (ret) {
		printk("kprobe register:%s faild\n",name);
		return -EINVAL;
	}

	if (!wait_event_interruptible_timeout(data.wait, data.kprobe_complete, 100*HZ))
		return -EBUSY;

    unregister_kprobe(&data.kp1);
	return 0;
}

int kprobe_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data)
{
	int ret = -1;
	char name[125];
	char *dump_buf;
	int len, dump_len;

	switch (data->cmdcode) {
		case  IOCTL_USEKRPOBE_FUNC_DUMP:
			dump_buf = kzalloc(PAGE_SIZE, GFP_KERNEL);
			len = data->kp_data.len;
			if (copy_from_user(name, (char __user *) data->kp_data.name, len)) {
				ERR("kprobe get sym name failed\n");
				return -ENOMEM;
			}
			name[len] = 0;
			ret = kprobe_func_dump(dump_buf, name);
			if (!ret) {
				dump_len = strlen(dump_buf);
				copy_to_user((char __user *) data->kp_data.dump_buf,  dump_buf,
						dump_len > data->kp_data.dump_len ? data->kp_data.dump_len : dump_len);
			}
			kfree(dump_buf);
			break;
		default:
			goto OUT;
	}
OUT:
	return ret;
}


int kprobe_init(void)
{
	orig_dump_stack_print_info = kallsyms_lookup_name("dump_stack_print_info");
	return 0;

}
