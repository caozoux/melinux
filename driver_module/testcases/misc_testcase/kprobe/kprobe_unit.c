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

typedef struct {
	struct list_head list;
	struct kprobe kp;
	char symname[128];
	int cpu;
} kprobe_item;

struct kprobe_misc_data {
	struct list_head head;
	struct kprobe kp1;
	struct kprobe kp_printk;
	struct kprobe kp_hook;
	struct completion kprobe_compl;
	wait_queue_head_t wait;
	u8 kprobe_complete;
	int cpu;
	char *dump_buf;
};

struct kprobe_misc_data  *kprobe_unit_data;

static int kprobe_post_handler(struct kprobe *p, struct pt_regs *regs, unsigned long flags);
static void (*orig_dump_stack_print_info)(const char *log_lvl);

static int add_kprobe_item(char *name, kprobe_pre_handler_t pre_func)
{
	kprobe_item *item;
	int ret;
	item = kzalloc(sizeof(kprobe_item), GFP_KERNEL);
	if (!item)
		return -ENOMEM; 

	INIT_LIST_HEAD(&item->list);
	strncpy(item->symname, name, 128);
	item->kp.symbol_name = item->symname;
	item->kp.pre_handler = pre_func;

	list_add_tail(&item->list, &kprobe_unit_data->head);
   	ret = register_kprobe(&item->kp);
	if (ret)
		goto out;

	return 0;
out:
	kfree(item);
	return -EINVAL;
}

static void remove_kprobe_item(kprobe_item *item)
{
   	unregister_kprobe(&item->kp);
	list_del(&item->list);
	kfree(item);
}

static int remove_kprobe_by_name(char *name)
{
	kprobe_item *item;
	int  i= 0;
	int ret = -EINVAL;

	list_for_each_entry(item, &kprobe_unit_data->head, list) {
		if(strstr(item->kp.symbol_name, name)) {
			remove_kprobe_item(item);
			ret = 0;
			break;
		}
	}
	return ret;
}

static int kprobe_post_handler(struct kprobe *p, struct pt_regs *regs, unsigned long flags)
{
	return 0;
}

static int kprobe_printk_handler(struct kprobe *p, struct pt_regs *regs)
{
	struct kprobe_misc_data *data = container_of(p, struct kprobe_misc_data, kp_printk);

	if (data->cpu != -1 && data->cpu == smp_processor_id())
		printk("zz %s %d \n", __func__, __LINE__);
	else
		printk("zz %s %d \n", __func__, __LINE__);
	return 0;
}

static int kprobe_hook_handler(struct kprobe *p, struct pt_regs *regs)
{
	struct kprobe_misc_data *data = container_of(p, struct kprobe_misc_data, kp_hook);
	if (data->cpu != -1 ) {
		if (data->cpu == smp_processor_id())
			return 1;
		return 0;
	}

	return 1;
}

static int kprobe_dump_stack_handler(struct kprobe *p, struct pt_regs *regs)
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

	if (!name) {
		printk("no symbole specified\n");
		return -EINVAL;
	}

	memset(kprobe_unit_data, 0, sizeof(struct kprobe_misc_data));
	init_completion(&kprobe_unit_data->kprobe_compl);
	init_waitqueue_head(&kprobe_unit_data->wait);
	kprobe_unit_data->kp1.symbol_name = name;
	kprobe_unit_data->kp1.pre_handler = kprobe_dump_stack_handler;

	kprobe_unit_data->dump_buf = dump_buf;

	if (!kprobe_unit_data->dump_buf)
		return -ENOMEM;

    ret = register_kprobe(&kprobe_unit_data->kp1);
	if (ret) {
		printk("kprobe register:%s faild\n",name);
		return -EINVAL;
	}

	if (!wait_event_interruptible_timeout(kprobe_unit_data->wait, kprobe_unit_data->kprobe_complete, 100*HZ))
		return -EBUSY;

    unregister_kprobe(&kprobe_unit_data->kp1);
	return 0;
}

int kprobe_unit_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data)
{
	int ret = -1;
	char name[128];
	char *dump_buf;
	int len, dump_len;
	//kprobe_item *item;

	len = data->kp_data.len;
	if (len <= 0)
		return -EINVAL;
	if (copy_from_user(name, (char __user *) data->kp_data.name, len)) {
		ERR("kprobe get sym name failed\n");
		return -ENOMEM;
	}
	name[len] = 0;

	switch (data->cmdcode) {
		case  IOCTL_USEKRPOBE_FUNC_DUMP:
			dump_buf = kzalloc(PAGE_SIZE, GFP_KERNEL);
			ret = kprobe_func_dump(dump_buf, name);
			if (!ret) {
				dump_len = strlen(dump_buf);
				if(copy_to_user((char __user *) data->kp_data.dump_buf,  dump_buf,
						dump_len > data->kp_data.dump_len ? data->kp_data.dump_len : dump_len))
					return -ENOMEM;
			}
			kfree(dump_buf);
			break;
		case  IOCTL_USEKRPOBE_KPROBE_HOOK:
			ret = add_kprobe_item(name, kprobe_hook_handler);
			if (ret) {
				printk("kprobe register:%s faild\n",name);
				return -EINVAL;
			}
			break;
		case  IOCTL_USEKRPOBE_KPROBE_FUNC:
			ret = add_kprobe_item(name, kprobe_printk_handler);
			if (ret) {
				printk("kprobe register:%s faild\n",name);
				return -EINVAL;
			}
			break;
		case  IOCTL_USEKRPOBE_KPROBE_FREE:
			ret = remove_kprobe_by_name(name);
			if (ret) {
				printk("kprobe free failed:%s faild\n",name);
				return -EINVAL;
			}
			break;
		default:
			goto OUT;
	}
OUT:
	return ret;
}


int kprobe_unit_init(void)
{
	kprobe_unit_data = kmalloc(sizeof(struct kprobe_misc_data), GFP_KERNEL);
	INIT_LIST_HEAD(&kprobe_unit_data->head);
	orig_dump_stack_print_info = (void*)kallsyms_lookup_name("dump_stack_print_info");
	return 0;
}

int kprobe_unit_exit(void)
{
	orig_dump_stack_print_info = (void*)kallsyms_lookup_name("dump_stack_print_info");
	return 0;

}
