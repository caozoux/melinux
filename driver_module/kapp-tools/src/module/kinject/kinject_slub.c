#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <ksioctl/kinject_ioctl.h>

#include "ksysdata.h"
#include "ksysd_ioctl.h"
#include "kinject_local.h"

LIST_HEAD(kmem_kmemcache_list);
struct kmem_kmemcache_item {
	struct list_head list;
	void *data;
};

struct kmem_kmemcache_data {
	struct list_head list;
	struct list_head item_list;
	char name[128];
	struct kmem_cache *kmem_cache;
};

struct kmem_kmemcache_data *kmem_kmemcache_get_item(char *name)
{
	struct kmem_kmemcache_data *data;
	list_for_each_entry(data, &kmem_kmemcache_list, list) {
		struct kmem_cache *s = data->kmem_cache;
		if (!strcmp(name, s->name))
			return data;
	}
	return NULL;
}

void kmem_kmemcache_remove(char *name)
{

	struct kmem_kmemcache_data *data;
	struct kmem_cache *s;

	data = kmem_kmemcache_get_item(name);
	if (!data)
		return;

	s = data->kmem_cache;
	if (!s)
		return;

	if (s->refcount > 1)
		goto failed;

	list_del(&data->list);
	kmem_cache_destroy(data->kmem_cache);

failed:
	pr_warn("%s objects:%d, can be free\n", s->name, s->refcount);

};

int kmem_kmemcache_create(char *name, int size)
{
	struct kmem_kmemcache_data *data;
	data = kmalloc(__GFP_ZERO, sizeof(struct kmem_kmemcache_data));
	if (!data)
		return -EINVAL;

	INIT_LIST_HEAD(&data->list);
	INIT_LIST_HEAD(&data->item_list);
	strcpy(data->name, name);

	data->kmem_cache = kmem_cache_create(data->name, size, 0,
						   SLAB_HWCACHE_ALIGN | SLAB_POISON,
						   NULL);
	if (!data->kmem_cache)
		goto out;

	list_add_tail(&data->list, &kmem_kmemcache_list);
	DBG("slub create:%s %p %s\n", data->kmem_cache->name, data->kmem_cache, data->name);
	return 0;

out:
	kfree(data);
	return -EINVAL;
}


int kinject_slub_init(void)
{
	return 0;
}

void kinject_slub_remove(void)
{

}
