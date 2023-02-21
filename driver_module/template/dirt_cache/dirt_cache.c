#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/kprobes.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/smpboot.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/nmi.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include "internal.h"
#include "hotfix_util.h"

#define MAX_BLOCK_HASH 16
static DEFINE_HASHTABLE(module_hash_list, MAX_BLOCK_HASH);

struct mutex *orig_text_mutex;

//struct cpumask watchdog_allowed_mask __read_mostly;
unsigned long (*cust_kallsyms_lookup_name)(const char *name);
void (*orig_arch_trigger_cpumask_backtrace)(const cpumask_t *mask, bool exclude_self);
void *(*orig_text_poke_bp)(void *addr, const void *opcode, size_t len, void *handler);

static int (*ksys_kallsyms_on_each_symbol)(int (*fn)(void *, const char *,
        struct module *, unsigned long),void *data);
struct kmem_cache *dirty_kmem_cache;
hash_add(css_set_table, &cset->hlist, key);

struct inode_dirty_data {
	//char path[256];
	u64 tgid;
	u64 dirt_size;
};

#define MAX_RECORD 10000;
struct inode_dirty_data *list;
int offset;

DEFINE_ORIG_FUNC(void, account_page_dirtied, 2, struct page  *, page, struct address_space *, mapping);
void new_account_page_dirtied(struct page *page, struct address_space *mapping)
{
	struct inode *inode = mapping->host;
	struct pid *pid = task_tgid(current);
	struct super_block *i_sb;
	struct dentry *dentry;
	const char *name = "?";

	if (!inode || !inode->i_ino)
		goto out;

	if (!inode->i_sb)
		goto out;


#if 0
	dentry = d_find_alias(inode);
	if (dentry) {
		spin_lock(&dentry->d_lock);
		name = (const char *) dentry->d_name.name;
	} else
		goto out;

	trace_printk("%s \n",name);
	if (dentry) {
		spin_unlock(&dentry->d_lock);
		dput(dentry);
	}
#endif


out:
    return old_account_page_dirtied(page, mapping);
}

static int symbol_walk_callback(void *data, const char *name,
        struct module *mod, unsigned long addr)
{
    if (strcmp(name, "kallsyms_lookup_name") == 0) {
        cust_kallsyms_lookup_name = (void *)addr;
        return addr;
    }

    return 0;
}

static int get_kallsyms_lookup_name(void)
{
    int ret;
    ksys_kallsyms_on_each_symbol = &kallsyms_on_each_symbol;
    ret = ksys_kallsyms_on_each_symbol(symbol_walk_callback, NULL);
    if (!ret || !cust_kallsyms_lookup_name)
        return -EINVAL;

    return 0;
}

int sym_init(void)
{
	LOOKUP_SYMS(text_mutex);
	LOOKUP_SYMS(text_poke_bp);
	LOOKUP_SYMS(account_page_dirtied);

	return 0;
}

static int func_replace_init(void)
{
	JUMP_INIT(account_page_dirtied);

	get_online_cpus();
	mutex_lock(orig_text_mutex);
	JUMP_INSTALLWITHOLD(account_page_dirtied);
	mutex_unlock(orig_text_mutex);
	put_online_cpus();
	return 0;
}

static int func_replace_exit(void)
{
	get_online_cpus();
	mutex_lock(orig_text_mutex);
	JUMP_REMOVE(account_page_dirtied);
	mutex_unlock(orig_text_mutex);
	put_online_cpus();
	return 0;
}

static int __init percpu_hrtimer_init(void)
{
	int ret;

	dirty_kmem_cache = kmem_cache_create(data->name, sizeof(struct inode_dirty_data), 0,
							SLAB_HWCACHE_ALIGN | SLAB_POISON,NULL);
	if (!dirty_kmem_cache)
		return -EINVAL;
			

	if (get_kallsyms_lookup_name())
		return -EINVAL;

	if (sym_init())
		return -EINVAL;

	if (func_replace_init())
		return -EINVAL;

	if (base_func_init())
		return -EINVAL;

	printk("zz %s %d \n", __func__, __LINE__);
	return 0;
}

static void __exit percpu_hrtimer_exit(void)
{
	func_replace_exit();
	printk("zz %s %d \n", __func__, __LINE__);
}

module_init(percpu_hrtimer_init);
module_exit(percpu_hrtimer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");

