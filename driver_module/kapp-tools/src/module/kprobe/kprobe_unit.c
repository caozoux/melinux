#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/kprobes.h>

#include "ksioctl/kprobe_ioctl.h"
#include "ksysdata.h"
#include "ksysd_ioctl.h"

typedef struct {
bool  kprobe_en;
bool  kretprobe_en;
struct kprobe kp;
struct kretprobe kretp;
char name[256];
unsigned long delay;
struct list_head list;
} kprobe_item;

static struct list_head kprobe_list;

static int __kprobes kprobe_prehandler(struct kprobe *p, struct pt_regs *regs);

static int kretprobe_handler(struct kretprobe_instance *ri, struct pt_regs *regs);

static int __kprobes kprobe_prehandler(struct kprobe *p, struct pt_regs *regs)
{
	kprobe_item *item = container_of(p, kprobe_item, kp);
	if (item && item->delay)
		udelay(item->delay);

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

//type: 
//    0 kprobe 
//    1 kretprobe 
//    2 kretprobe  and kprobe
static int kprobe_register_function(char *name, int type, unsigned long delay)
{
	kprobe_item *item;
	list_for_each_entry(item, &kprobe_list, list) {
		if (strcmp(item->name, name)) {
			printk("Warning: %s has register\n", name);
			return 0;
		}
	}

	item = kzalloc(sizeof(kprobe_item), GFP_KERNEL);
	if (!item)
		return -ENOMEM;
	strncpy(item->name, name, 256);
	item->delay = delay;
	printk("zz %s delay:%lx \n",__func__, (unsigned long)item->delay);

	switch (type) {
		case 0:
			item->kp.pre_handler  = kprobe_prehandler,
			item->kp.symbol_name = item->name;
			if (register_kprobe(&item->kp)) {
				printk("Warning: %s register failed\n", name);
				goto out;
			}
			item->kprobe_en = true;
			break;
		case 1:
			item->kretp.handler  = kretprobe_handler,
			item->kretp.kp.symbol_name = item->name;
			if (register_kretprobe(&item->kretp)) {
				printk("Warning: %s register failed\n", name);
				goto out;
			}
			item->kretprobe_en = true;
			break;
		case 2:
			item->kp.pre_handler  = kprobe_prehandler,
			item->kp.symbol_name = item->name;
			if (register_kprobe(&item->kp)) {
				printk("Warning: %s register failed\n", name);
				goto out;
			}
			item->kprobe_en = true;

			item->kretp.handler  = kretprobe_handler,
			item->kretp.kp.symbol_name = item->name;
			if (register_kretprobe(&item->kretp)) {
				unregister_kprobe(&item->kp);
				item->kprobe_en = false;
				printk("Warning: %s register failed\n", name);
				goto out;
			}
			item->kretprobe_en = true;
			break;
		default:
			break;
	}

	list_add_tail(&item->list, &kprobe_list);
	return 0;
out:
	kfree(item);
	return -EINVAL;
}

int kprobe_unit_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_ksdata *data)
{
	int ret = 0;
	struct kprobe_ioctl kioctl;

	DBG("kprobe subcmd:%d\n", (int)data->subcmd);
	if (copy_from_user(&kioctl, (char __user *)data->data, sizeof(struct kprobe_ioctl))) {
		printk("ioctl data copy err\n");
		ret = -EFAULT;
		goto OUT;
	}

	switch (data->subcmd) {
		case IOCTL_KRPOBE_FUNC_DUMP:
			break;
		case IOCTL_KRPOBE_FUNC_DUMP_CLEAN:
			break;
		case  IOCTL_KPROBE_SUB_DELAY:
			ret = kprobe_register_function(kioctl.name, 0, kioctl.delay);
			break;
		case  IOCTL_KPROBE_SUB_LIST:
			break;
		default:
			break;
	}

OUT:
	return ret;
}

int kprobe_unit_init(void)
{
	INIT_LIST_HEAD(&kprobe_list);
	return 0;
}

int kprobe_unit_exit(void)
{
	kprobe_item *item, *n;
	list_for_each_entry_safe(item, n, &kprobe_list, list) {
		printk("zz %s name:%s \n",__func__, item->name);
		if (item->kprobe_en)
			unregister_kprobe(&item->kp);
		if (item->kretprobe_en)
			unregister_kretprobe(&item->kretp);
		list_del(&item->list);
		kfree(item);
	}
	return 0;
}

