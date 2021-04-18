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
#include "kmemlocal.h"

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
		return -EINVAL;

	s = data->kmem_cache;
	if (!s)
		return;

	if (s->refcount > 1)
		goto failed;

	list_del(&data->list);
	kmem_cache_destroy(data->kmem_cache);

failed:
	pr_warn("%s objects:%ld, can be free\n", s->name, s->refcount);

};

int kmem_kmemcache_create(char *name, int size)
{
	int i;
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
	DEBUG("slub create:%s %lx %s\n", data->kmem_cache->name, data->kmem_cache, data->name);
	return 0;

out:
	kfree(data);
	return -EINVAL;
}

int kmem_kmemcache_create_objs(char *name, int size, int is_free)
{
	struct kmem_kmemcache_data *data;
	struct kmem_cache *s;
	int i;
	struct kmem_kmemcache_item *item;
	struct list_head *list;

	data = kmem_kmemcache_get_item(name);
	if (!data) {
		pr_warn("%s get failed\n", name);
		return -EINVAL;
	}

	s = data->kmem_cache;

	DEBUG("slub %lx size:%d\n", data->kmem_cache->name, size);

	if (is_free) {
		i = 0;
		while((!list_empty(&data->item_list)) && i < size) {
			list = data->item_list.next;	
			item = container_of(list, struct kmem_kmemcache_item, list);
			list_del(list);
			kmem_cache_free(s, item);
			i++;
		}
	} else {
		for (i = 0; i < size; ++i) {
			item = kmem_cache_alloc(s, GFP_KERNEL);
			if (!item)
				continue;

			INIT_LIST_HEAD(&item->list);
			list_add_tail(&item->list, &data->item_list);
		}
	}
}

