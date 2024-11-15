#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <ksioctl/kinject_ioctl.h>
#include <linux/rhashtable.h>

#include "ksysdata.h"
#include "ksysd_ioctl.h"
#include "kinject_local.h"

#define MAX_SIZE (1UL<<20)

struct hash_entry {
	struct rhash_head node;
	u32 key;
	u64 value;
};
static struct rhashtable rht;

void hash_entry_free(void *ptr, void *arg)
{
	kfree(ptr);
}

static int rhashtable_test1_init(void)
{
	u32 i;
	struct hash_entry *entry;
	struct rhashtable_params param = {
		.key_len = sizeof(u32),
		.key_offset = offsetof(struct hash_entry, key),
		.head_offset = offsetof(struct hash_entry, node),
		.automatic_shrinking = true,
	};
	int ret;

	ret = rhashtable_init(&rht, &param);
	if (ret < 0)
		return ret;

	printk("rhashtable_init %lx\n", (unsigned long)&rht);
#if 1

	for (i = 0; i < MAX_SIZE; ++i) {
		entry = kzalloc(sizeof(struct hash_entry), GFP_KERNEL);
		if (entry == NULL) {
			printk("kzalloc returns NULL\n");
			goto err_exit;
		}
		entry->key = i;
		entry->value = (u64)i * i;
		//printk("Inserting %u %llu\n", entry->key, entry->value);
		ret = rhashtable_insert_fast(&rht, &entry->node, param);
		if (ret < 0) {
			kfree(entry);
			printk("rhashtable_insert_fast returns %d\n", ret);
			goto err_exit;
		}
	}

	for (i = 0; i < MAX_SIZE; ++i) {
		// 如果可以保证entry不被删掉，那么可以用rhashtable_lookup_fast
		entry = rhashtable_lookup_fast(&rht, &i, param);
		if (entry == NULL) {
			printk("rhashtable_lookup_fast returns NULL\n");
			goto err_exit;
		}

		if (entry->value != (u64)i * i) {
			printk("%u %llu\n", i, entry->value);
			goto err_exit;
		}

		// 如果另一个线程可能会删掉entry，那么需要一直拿着read lock直到不再需要entry
		rcu_read_lock();
		entry = rhashtable_lookup(&rht, &i, param);
		if (entry == NULL) {
			printk("rhashtable_lookup returns NULL\n");
			goto err_exit;
		}
		if (entry->value != (u64)i * i) {
			printk("%u %llu\n", i, entry->value);
			goto err_exit;
		}
		rcu_read_unlock();
	}
err_exit:
#endif
	return 0;
}

static void rhashtable_test1_exit(void)
{
	rhashtable_free_and_destroy(&rht, hash_entry_free, NULL);

}

int kinject_rhashtable_func(enum IOCTL_INJECT_SUB cmd, struct kinject_ioctl *data)
{
	switch (cmd) {
		default:
			break;
	}

	return 0;
}

int kinject_rhashtable_int(void)
{
	rhashtable_test1_init();
	return 0;
}

void kinject_rhashtable_exit(void)
{
	rhashtable_test1_exit();
}

