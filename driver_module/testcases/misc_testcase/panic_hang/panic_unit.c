#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/slab.h>
#include <linux/slub_def.h>
#include <linux/delay.h>
#include <linux/swap.h>
#include <linux/swapops.h>
#include <linux/page_idle.h>
#include <linux/version.h>
//#include <asm/tlb.h>

#include "../template_iocmd.h"
#include "../misc_ioctl.h"
#include "../debug_ctrl.h"
#include "mekernel.h"

static char **orig_log_buf;
static u32 *orig_log_buf_len;
static u64 *orig_log_next_seq;
static u32 *orig_log_next_idx;
static u64 *orig_log_first_seq;
static u32 *orig_log_first_idx;

struct kprobe rcu_kprobe;

static int rcu_stall_kprobe(struct kprobe *p, struct pt_regs *regs)
{

	return 0;
}

struct printk_log {
	u64 ts_nsec;        /* timestamp in nanoseconds */
	u16 len;        /* length of entire record */
	u16 text_len;       /* length of text buffer */
	u16 dict_len;       /* length of dictionary buffer */
	u8 facility;        /* syslog facility */
	u8 flags:5;     /* internal record flags */
	u8 level:3;     /* syslog level */
};

static char *log_text(const struct printk_log *msg)
{
	return (char *)msg + sizeof(struct printk_log);
}

static u32 log_next(u32 idx)
{
	struct printk_log *msg = (struct printk_log *)(*orig_log_buf + idx);

	if (!msg->len) {
		msg = (struct printk_log *)orig_log_buf;
		return msg->len;
	}

	return idx + msg->len;

}

static struct printk_log *log_from_idx(u32 idx)
{
	struct printk_log *msg = (struct printk_log *)(*orig_log_buf + idx);

	if (!msg->len)
		return (struct printk_log *) orig_log_buf;

	return msg;
}

static void dump_panic_log(void)
{
	u64 dump_seq;
	u64 dump_idx;
	struct printk_log *msg;
	char *print_buf;
	u64 print_len = 0;
	int index;

	print_buf = kmalloc(0x100000, GFP_KERNEL);

	//while (log_first_seq < log_next_seq
	//char buf[0x1000];
	//memcpy(buf, *orig_log_buf, 0x1000);
	printk("zz %s orig_log_next_idx:%lx orig_log_next_seq:%lx \n",__func__, (unsigned long)*orig_log_next_idx, (unsigned long)*orig_log_next_seq);
	printk("zz %s orig_log_first_seq:%lx orig_log_first_idx:%lx \n",__func__, (unsigned long)orig_log_first_seq, (unsigned long)orig_log_first_idx);
	printk("zz %s orig_log_buf:%lx orig_log_buf_len:%lx \n",__func__, (unsigned long)*orig_log_buf, (unsigned long)*orig_log_buf_len);

	//printk("%s\n", buf);
	dump_seq = *orig_log_first_seq;
	dump_idx = *orig_log_first_idx;
	while(1) {
		msg = log_from_idx(dump_idx);
		//printk("zz %s msg->len:%lx msg->text_len:%lx \n",__func__, (unsigned long)msg->len, (unsigned long)msg->text_len);
		//printk("zz %s msg:%s \n",__func__, log_text(msg));
		trace_printk("%s\n", log_text(msg));
		if (dump_seq == *orig_log_next_seq)
			break;

		memcpy(print_buf + print_len, log_text(msg), msg->text_len);
		print_len += msg->text_len;
		dump_idx = log_next(dump_idx);
		dump_seq++;
	}

	printk("%s\n", print_buf);
	kfree(print_buf);

}

static int dump_mekmem_printk(struct notifier_block *self, unsigned long v, void *p)
{
	pr_emerg("zz %s %d \n", __func__, __LINE__);
    return 0;
}

static struct notifier_block dump_mekmem_notifier = {
    .notifier_call = dump_mekmem_printk,
};


int panic_unit_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_data *data)
{
	int ret;

	switch (data->cmdcode) {
		case IOCTL_PANIC_NOTIFIER:
			ret = atomic_notifier_chain_register(&panic_notifier_list,
					&dump_mekmem_notifier);
			DEBUG("panic notfier %s\n", ret ? "failed" : "successful");
			break;

		case IOCTL_PANIC_UNNOTIFIER:
			DEBUG("panic unnotfier\n");
			atomic_notifier_chain_unregister(&panic_notifier_list,
					&dump_mekmem_notifier);
			break;

		case IOCTL_PANIC_TRIGGER:
			DEBUG("panic trigger\n");
			panic("trigger panic");
			break;

		case IOCTL_PANIC_LOG:
			DEBUG("panic dump\n");
			dump_panic_log();
			break;

		default:
			break;
	}

	return 0;
}

int panic_unit_init(void)
{
	LOOKUP_SYMS(log_buf);
	LOOKUP_SYMS(log_buf_len);
	LOOKUP_SYMS(log_next_idx);
	LOOKUP_SYMS(log_next_seq);
	LOOKUP_SYMS(log_first_idx);
	LOOKUP_SYMS(log_first_seq);
	return 0;
}

int panic_unit_exit(void)
{
	return 0;
}

