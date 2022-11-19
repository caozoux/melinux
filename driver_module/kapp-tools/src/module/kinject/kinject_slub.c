#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/slub_def.h>
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

struct kmem_kmemcache_data *kinject_kmem;

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

#if 0
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

struct kmem_kmemcache_data * kmem_kmemcache_create(char *name, int size)
{

	strcpy(data->name, name);

	data->kmem_cache = kmem_cache_create(data->name, size, 0,
						   SLAB_HWCACHE_ALIGN | SLAB_POISON,
						   NULL);
	if (!data->kmem_cache)
		goto out;

	list_add_tail(&data->list, &kmem_kmemcache_list);
	DBG("slub create:%s %p %s\n", data->kmem_cache->name, data->kmem_cache, data->name);
	return data;

out:
	kfree(data);
	return NULL;
}
#endif

int kinject_slub_overwrite(void)
{
	char *data;
	data = kmem_cache_zalloc(kinject_kmem->kmem_cache, GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data[65] = 0;
	//kmem_cache_free(kinject_kmem->kmem_cache, data);
	return 0;
}

int kinject_slub_init(void)
{
	kinject_kmem = kzalloc(__GFP_ZERO, sizeof(struct kmem_kmemcache_data));
	if (!kinject_kmem)
		return -1;
	INIT_LIST_HEAD(&kinject_kmem->list);
	return 0;
}

void kinject_slub_remove(void)
{
	if (kinject_kmem->kmem_cache)
		kmem_cache_destroy(kinject_kmem->kmem_cache);

	kfree(kinject_kmem);
}

int kinject_slub_func(enum IOCTL_INJECT_SUB cmd, struct kinject_ioctl *data)
{
	switch (cmd) {
		case IOCTL_INJECT_SLUB_CTRL:
			if (data->enable) {
				kinject_kmem->kmem_cache = kmem_cache_create("kinject_64", 64, 0,
						   SLAB_HWCACHE_ALIGN | SLAB_POISON,
						   NULL);
				if (!kinject_kmem->kmem_cache)
					return -EINVAL;
			} else {
				kmem_cache_destroy(kinject_kmem->kmem_cache);
			}
			break;
		case IOCTL_INJECT_SLUB_OVERWRITE:
			if (!kinject_kmem->kmem_cache)
				return -EINVAL;
			kinject_slub_overwrite();
			break;
		default:
			break;
	}

	return 0;
}

