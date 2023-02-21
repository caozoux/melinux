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

static DEFINE_HASHTABLE(module_hash_table, CSS_SET_HASH_BITS);
struct kmem_cache *dirty_kmem_cache;
hash_add(css_set_table, &cset->hlist, key);

static int __init kprobe_driver_init(void)
{

	return 0;
}

static void __exit kprobe_driver_exit(void)
{
}

module_init(kprobe_driver_init);
module_exit(kprobe_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
