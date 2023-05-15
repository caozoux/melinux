#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/ip.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/kprobes.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/skbuff.h>
#include <linux/udp.h>
#include <linux/netdevice.h>
#include <linux/syscore_ops.h>
#include <linux/proc_fs.h>
#include <linux/jbd2.h>


static int kretprobe_handble(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	int ret = regs->ax;
	if (ret >= 0)
		printk("zz %s %d ret:%d\n", __func__, __LINE__, ret);

	return 0;
}

struct kretprobe kretlist[]  = {
	{
		.kp.symbol_name = "cpu_cgroup_allowed_cpu",
		.handler= kretprobe_handble,
	},
};

static int __init kprobe_driver_init(void)
{
	int i;
	for (i = 0; i < sizeof(kretlist)/sizeof(struct kprobe); ++i) {
		if (register_kretprobe(&kretlist[i])) {
			while (i>0) {
				i--;
				unregister_kretprobe(&kretlist[i]);
			}
			goto out;
		}
	}
	return 0;
out:
	return -EINVAL;
}

static void __exit kprobe_driver_exit(void)
{
	int i;
	for (i = 0; i < sizeof(kretlist)/sizeof(struct kprobe); ++i) {
		unregister_kretprobe(&kretlist[i]);
	}
}

module_init(kprobe_driver_init);
module_exit(kprobe_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
