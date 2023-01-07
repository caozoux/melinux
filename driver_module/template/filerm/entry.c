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

struct mutex *orig_text_mutex;

unsigned long (*cust_kallsyms_lookup_name)(const char *name);
void (*orig_arch_trigger_cpumask_backtrace)(const cpumask_t *mask, bool exclude_self);
void *(*orig_text_poke_bp)(void *addr, const void *opcode, size_t len, void *handler);

static int (*ksys_kallsyms_on_each_symbol)(int (*fn)(void *, const char *,
        struct module *, unsigned long),void *data);

DEFINE_ORIG_FUNC(int, vfs_rmdir, 2, struct inode *, dir, struct dentry *, dentry);
int new_vfs_rmdir(struct inode *dir, struct dentry *dentry)
{

	if (dentry && dentry->d_inode) {
		if (S_ISDIR(dentry->d_inode->i_mode)) {
			if (strncmp( dentry->d_iname, "kess", 4)) {
				printk("kat task:%s tgid:%d \n", current->comm, current->tgid);
			}
		}
	}

    return old_vfs_rmdir(dir, dentry);
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
	LOOKUP_SYMS(vfs_rmdir);

	return 0;
}

static int func_replace_init(void)
{
	JUMP_INIT(vfs_rmdir);

	get_online_cpus();
	mutex_lock(orig_text_mutex);
	JUMP_INSTALLWITHOLD(vfs_rmdir);
	mutex_unlock(orig_text_mutex);
	put_online_cpus();
	return 0;
}

static int func_replace_exit(void)
{
	get_online_cpus();
	mutex_lock(orig_text_mutex);
	JUMP_REMOVE(vfs_rmdir);
	mutex_unlock(orig_text_mutex);
	put_online_cpus();
	return 0;
}

static int __init percpu_hrtimer_init(void)
{

	if (get_kallsyms_lookup_name())
		return -EINVAL;

	if (sym_init())
		return -EINVAL;

	if (func_replace_init())
		return -EINVAL;

	//ret = register_tracepoint("writeback_dirty_page", account_page_dirtied_trace, NULL);
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

