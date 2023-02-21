#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/kprobes.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/dcache.h>
#include <linux/smpboot.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/nmi.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>

unsigned long (*cust_kallsyms_lookup_name)(const char *name);
struct hrtimer hrtimer_pr;
int *orig_css_set_count;

struct list_head *origme__cgrp_cpuctx_list;
static DEFINE_PER_CPU(struct list_head, me_cgrp_cpuctx_list);
static enum hrtimer_restart hrtimer_pr_fun(struct hrtimer *hrtimer)
{
  	trace_printk("zz css_set_count:%d \n", *orig_css_set_count);
	hrtimer_forward_now(&hrtimer_pr, ns_to_ktime(1000000000));
	return HRTIMER_RESTART;
}

static int (*ksys_kallsyms_on_each_symbol)(int (*fn)(void *, const char *,
        struct module *, unsigned long),void *data);

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

  orig_css_set_count = (void *)cust_kallsyms_lookup_name("css_set_count");
  if (!orig_css_set_count)
	return -EINVAL;

  origme__cgrp_cpuctx_list= (void *)cust_kallsyms_lookup_name("cgrp_cpuctx_list");
  if (!origme__cgrp_cpuctx_list)
	return -EINVAL;

  return 0;
}

static int hrtimer_pr_init(void)
{
	return 0;
}

static void hrtimer_pr_exit(void)
{
	hrtimer_cancel(&hrtimer_pr);
}

void dump_dentry(struct inode *inode)
{
        struct dentry *dentry;
        const char *name = "?";
        dentry = d_find_alias(inode);
        if (dentry) {
            spin_lock(&dentry->d_lock);
            name = (const char *) dentry->d_name.name;
        }
        printk( "dirtied inode %lu (%s) on \n",
               inode->i_ino,
               name);
        if (dentry) {
            spin_unlock(&dentry->d_lock);
            dput(dentry);
        }
}

static int __init percpu_hrtimer_init(void)
{

	struct super_block *sb = 0xffff9ae5e440a548;
	struct inode *inode;
  if (get_kallsyms_lookup_name())
    return -EINVAL;

  if (sym_init())
    return -EINVAL;

	spin_lock(&sb->s_inode_list_lock);
	list_for_each_entry(inode, &sb->s_inodes, i_sb_list) {
		printk("zz %s inode:%lx \n",__func__, (unsigned long)inode);
		dump_dentry(inode);
	}
	spin_unlock(&sb->s_inode_list_lock);
	return 0;
}

static void __exit percpu_hrtimer_exit(void)
{
  //hrtimer_pr_exit();	
}

module_init(percpu_hrtimer_init);
module_exit(percpu_hrtimer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
