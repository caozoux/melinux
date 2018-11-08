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

#define MODULE_CACHE_NAME "module_slub_cache"
#define MODULE_FLAG_CACHE_NAME "module_slub_flagrcu_cache"

struct kmem_cache *new_kmem_cache;
struct kmem_cache *flag_kmem_cache;

#define ALLOCE_SIZE 1024
#define FREE_SIZE   512
static void *addr[ALLOCE_SIZE];

static void kemcache_alloc_test_init(void)
{
	int i;
	for (i = 0; i < ALLOCE_SIZE; ++i) {
		addr[i] = kmem_cache_alloc(new_kmem_cache, GFP_KERNEL);
		printk("zz %s addr:%lx \n",__func__, (long int)addr[i]);
	}
		
	for (i = 0; i < FREE_SIZE; ++i) {
		kmem_cache_free(new_kmem_cache, addr[i]);
	}
}

static void kemcache_alloc_test_exit(void)
{
	int i;
	for (i = FREE_SIZE; i < ALLOCE_SIZE; ++i) {
		kmem_cache_free(new_kmem_cache, addr[i]);
	}
}

static int kmemcache_init(void)
{
	new_kmem_cache = kmem_cache_create(MODULE_CACHE_NAME, 1044, 0,
						   SLAB_HWCACHE_ALIGN,
						   NULL);

	kmem_cache_shrink(new_kmem_cache);
	//kmem_dump_zone();
	return 0;
}

static void kmemcache_exit(void)
{
	if (new_kmem_cache)
		kmem_cache_destroy(new_kmem_cache);
}

static int kmemcache_flag_rcu_init(void)
{
	flag_kmem_cache = kmem_cache_create(MODULE_CACHE_NAME, 1044, 0,
				SLAB_DESTROY_BY_RCU,
						   NULL);

	if(!flag_kmem_cache)
		return 1;
}

static int kmemcache_flag_rcu_exit(void)
{
	if (flag_kmem_cache)
		kmem_cache_destroy(flag_kmem_cache);
	return 0;
}

