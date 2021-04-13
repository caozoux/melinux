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
	struct kmem_cache *kmem_cache;
};

void kmemcache_remove(struct kmem_kmemcache_data *data)
{
	struct kmem_cache *s = data->kmem_cache;
	if (!s)
		return;

	if (s->refcount > 1)
		goto failed;

	list_del(&data->list);
	kmem_cache_destroy(data->kmem_cache);

failed:
	pr_warn("%s objects:%ld, can be free\n", s->name, s->refcount);

};

int kmemcache_create(char *name, int size)
{
	int i;
	struct kmem_kmemcache_data *data;
	data = kmalloc(__GFP_ZERO, sizeof(struct kmem_kmemcache_data));
	if (!data)
		return -EINVAL;

	INIT_LIST_HEAD(&data->list);

	data->kmem_cache = kmem_cache_create(name, size, 0,
						   SLAB_HWCACHE_ALIGN,
						   NULL);
	if (!data->kmem_cache)
		goto out;

	list_add_tail(&kmem_kmemcache_list, &data->list);
	return 0;

out:
	kfree(data);
	return -EINVAL;
}

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

int kmem_kmemcache_create_objs(char *name, int size, int is_free)
{
	struct kmem_kmemcache_data *data;
	struct kmem_cache *s;
	int i;
	struct kmem_kmemcache_item *item;

	data = kmem_kmemcache_get_item(name);
	if (!data)
		return -EINVAL;
		
	s = data->kmem_cache;

	if (is_free) {
		i = 0;
		list_for_each_entry(item, &data->item_list, list) {
			list_del(&item->list);
			kmem_cache_free(s, item);
			i++;
			if (i >= size)
				break;
		}
	} else {
		for (i = 0; i < size; ++i) {
			item = kmem_cache_alloc(s, GFP_KERNEL);
			if (!item)
				continue;

			INIT_LIST_HEAD(&item->list);
			list_add_tail(&data->item_list, &item->list);
		}
	}
}

