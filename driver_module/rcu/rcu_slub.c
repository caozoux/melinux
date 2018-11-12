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
#include <linux/syscore_ops.h>
#include <linux/syscalls.h>
#include  "rcu_module.h"

struct kmem_cache *flag_kmem_cache;
#define MODULE_FLAG_CACHE_NAME "module_slub_flagrcu_cache"

int __maybe_unused kmemcache_flag_rcu_init(void)
{
	flag_kmem_cache = kmem_cache_create(MODULE_FLAG_CACHE_NAME, 1044, 0,
				SLAB_DESTROY_BY_RCU,
						   NULL);

	if(!flag_kmem_cache)
		return 1;
}

int __maybe_unused kmemcache_flag_rcu_exit(void)
{
	if (flag_kmem_cache)
		kmem_cache_destroy(flag_kmem_cache);
	return 0;
}
