#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>

#define NEW_CACHE_NAME "metest64"

struct kmem_cache *new_kmem_cache;
static void *addr[12];

static int __init kmemcachedriver_init(void)
{
	int i;
	new_kmem_cache = kmem_cache_create(NEW_CACHE_NAME, 93, 0,
						   SLAB_HWCACHE_ALIGN | SLAB_PANIC | SLAB_NOTRACK ,
						   NULL);

	printk("zz %s new_kmem_cache:%lx \n",__func__, (long int)new_kmem_cache);

	for (i = 0; i < 12; ++i) {
		addr[i] = kmem_cache_alloc(new_kmem_cache, GFP_KERNEL);
		printk("zz %s addr:%lx \n",__func__, (long int)addr[i]);
	}
		
	printk("kmemcachedriver load \n");
	return 0;
}

static void __exit kmemcachedriver_exit(void)
{
	int i;
	for (i = 0; i < 12; ++i) {
		kmem_cache_free(new_kmem_cache, addr[i]);
	}
	kmem_cache_destroy(new_kmem_cache);
	printk("kmemcachedriver unload \n");
}

module_init(kmemcachedriver_init);
module_exit(kmemcachedriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
