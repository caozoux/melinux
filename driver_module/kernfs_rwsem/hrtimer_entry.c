#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/kprobes.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>

#define TIME_INTERVAL (1000000000)
struct hrtimer g_hrtimer;

#define LOOKUP_SYMS(name) do { \
		orig_##name = (void *)cust_kallsyms_lookup_name(#name);		\
		if (!orig_##name) {						\
			pr_err("kallsyms_lookup_name: %s\n", #name);		\
			return -EINVAL;						\
		}								\
	} while (0)

unsigned long (*cust_kallsyms_lookup_name)(const char *name);
struct rw_semaphore * orig_kernfs_rwsem;

static int sysm_lookup_init(void)
{
	LOOKUP_SYMS(kernfs_rwsem);
	
	return 0;
}

static int noop_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
    return 0;
}

static int sysm_init(void)
{
    struct kprobe kp;
	int ret;

	memset(&kp, 0, sizeof(struct kprobe));
    ret = -1;
    kp.symbol_name = "kallsyms_lookup_name";
    kp.pre_handler = noop_pre_handler;

    ret = register_kprobe(&kp);
    if (ret < 0) {
        return -EINVAL;
    }

    cust_kallsyms_lookup_name = (void*)kp.addr;
    unregister_kprobe(&kp);

	if (sysm_lookup_init())
		return -EINVAL;

	return 0;
}

static enum hrtimer_restart watchdog_timer_fn(struct hrtimer *hrtimer)
{
	hrtimer_forward_now(hrtimer, ns_to_ktime(TIME_INTERVAL));
	trace_printk("zz %s %d \n", __func__, __LINE__);
	return HRTIMER_RESTART;
}

static void hrtimer_entry_init(void)
{
	hrtimer_init(&g_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	g_hrtimer.function = watchdog_timer_fn;
	hrtimer_start(&g_hrtimer, ns_to_ktime(TIME_INTERVAL),
			HRTIMER_MODE_REL_PINNED);
}

static int __init hrtimerdriver_init(void)
{
	if (sysm_init())
		return -EINVAL;
	hrtimer_entry_init();
	return 0;
}

static void __exit hrtimerdriver_exit(void)
{
	hrtimer_cancel(&g_hrtimer);
}

module_init(hrtimerdriver_init);
module_exit(hrtimerdriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
