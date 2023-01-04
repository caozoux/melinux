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

static DEFINE_HASHTABLE(css_set_table, CSS_SET_HASH_BITS);
struct kd_percpu_data kd_percpu_data[512];

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
	struct hlist_node hlist;
	u64 dirty_size;
	u64 i_ino;
	char *path[256];
	u64 pid[256];
	u64 size[256];
};

DEFINE_ORIG_FUNC(void, account_page_dirtied, 2, struct page  *, page, struct address_space *, mapping);
void new_account_page_dirtied(struct page *page, struct address_space *mapping)
{
	struct inode *inode = mapping->host;
	struct pid *pid = task_tgid(current);
	struct dentry *dentry;
	const char *name = "?";

	//item = kmem_cache_alloc(s, GFP_KERNEL);
	//kmem_cache_free(s, item);

	if (!inode || !inode->i_ino)
		goto out;

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

	//mm = get_task_mm(tsk);

out:
    return old_account_page_dirtied(page, mapping);
}

void account_page_dirtied_trace(void *data, u64 *var, unsigned int var_ref_idx)
{
	struct test_data *trace_data;
	trace_data = (struct test_data *)var;
	printk("zz %s var:%lx var_ref_idx:%lx \n",__func__, (unsigned long)var, (unsigned long)var_ref_idx);

	//printk("zz %s var:%lx \n",__func__, (unsigned long)var[var_ref_idx]);
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

	//ret = register_tracepoint("writeback_dirty_page", account_page_dirtied_trace, NULL);
	printk("zz %s %d \n", __func__, __LINE__);
	return 0;
}

static void __exit percpu_hrtimer_exit(void)
{
	func_replace_exit();
	//unregister_tracepoint("writeback_dirty_page", account_page_dirtied_trace, NULL);
	printk("zz %s %d \n", __func__, __LINE__);
}

module_init(percpu_hrtimer_init);
module_exit(percpu_hrtimer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");

