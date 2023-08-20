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
#include <trace/events/block.h>

#define LOOKUP_SYMS(name) do {							\
		orig_##name = (void *)cust_kallsyms_lookup_name(#name);		\
		if (!orig_##name) {						\
			pr_err("kallsyms_lookup_name: %s\n", #name);		\
			return -EINVAL;						\
		}								\
	} while (0)

unsigned long (*cust_kallsyms_lookup_name)(const char *name);

static int (*ksys_kallsyms_on_each_symbol)(int (*fn)(void *, const char *,
        struct module *, unsigned long),void *data);

struct tracepoint *orig___tracepoint_writeback_dirty_page;
DEFINE_STATIC_KEY_FALSE(dump_enabled_key);

noinline void dump_dentry(char *parent, char *child)
{
	if (static_branch_unlikely(&dump_enabled_key))
		trace_printk("%s %s/%s", blkname, parent, child);
	//trace_printk("%s/%s", parent, child);
}
EXPORT_SYMBOL(dump_dentry);

void trace_account_page_dirtied(void * ignore, struct page *page, struct address_space *mapping)
{
	struct inode *inode = mapping->host;
	struct super_block *i_sb;
	struct dentry *dentry, *parent;

	if (!inode || !inode->i_ino)
		return;

	if (!inode->i_sb)
		return;

	i_sb = inode->i_sb;

    dentry = d_find_alias(inode);
	if (!dentry)
        return;

    parent = dget_parent(dentry);
	if (!parent)
        goto out_dput;

	dump_dentry(i_sb->s_id, parent->d_iname, dentry->d_iname);
	dput(parent);
out_dput:
	dput(dentry);
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
	LOOKUP_SYMS(__tracepoint_writeback_dirty_page);
	return 0;
}

static int __init percpu_hrtimer_init(void)
{
	int ret;

	if (get_kallsyms_lookup_name())
		return -EINVAL;

	if (sym_init())
		return -EINVAL;

	ret = tracepoint_probe_register(orig___tracepoint_writeback_dirty_page, trace_account_page_dirtied, NULL);
	if (ret)
		return -EINVAL;

	return 0;
}

static void __exit percpu_hrtimer_exit(void)
{
	tracepoint_probe_unregister(orig___tracepoint_writeback_dirty_page, trace_account_page_dirtied, NULL);
}

module_init(percpu_hrtimer_init);
module_exit(percpu_hrtimer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");

