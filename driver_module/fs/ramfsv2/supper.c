#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/highmem.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/backing-dev.h>
#include <linux/sched.h>
#include <linux/parser.h>
#include <linux/magic.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/iversion.h>
#include "ramfsv2_inode.h"
#include "internal.h"

struct kmem_cache *ramfsv2_inode_cachep;

static inline struct ramfsv2_inode_info *RAMFSV2_I(struct inode *inode)
{
	return container_of(inode, struct ramfsv2_inode_info, vfs_inode);
}
/*
 * Display the mount options in /proc/mounts.
 */

static int ramfs_show_options(struct seq_file *m, struct dentry *root)
{
	struct ramfs_fs_info *fsi = root->d_sb->s_fs_info;

	if (fsi->mount_opts.mode != RAMFS_DEFAULT_MODE)
		seq_printf(m, ",mode=%o", fsi->mount_opts.mode);
	return 0;
}

static struct inode *ramfsv2_alloc_inode(struct super_block *sb)
{
	struct ramfsv2_inode_info *ei;

	ei = kmem_cache_alloc(ramfsv2_inode_cachep, GFP_NOFS);
	if (!ei)
		return NULL;
	inode_set_iversion(&ei->vfs_inode, 1);
	printk("zz %s vfs_inode:%lx \n",__func__, (unsigned long)&ei->vfs_inode);
	return &ei->vfs_inode;
}

static void ramfsv2_i_callback(struct rcu_head *head)
{
	struct inode *inode = container_of(head, struct inode, i_rcu);
	kmem_cache_free(ramfsv2_inode_cachep, RAMFSV2_I(inode));
	printk("zz %s %d \n", __func__, __LINE__);
}

static void ramfsv2_destroy_inode(struct inode *inode)
{
	printk("zz %s %d \n", __func__, __LINE__);
	call_rcu(&inode->i_rcu, ramfsv2_i_callback);
}

static int ramfsv2_write_inode(struct inode *inode, struct writeback_control *wbc)
{
	printk("zz %s %d \n", __func__, __LINE__);
	return 0;	
}

static void ramfsv2_dirty_inode(struct inode *inode, int flags)
{
	//struct buffer_head *ibh;
	printk("zz %s %d \n", __func__, __LINE__);
	//mark_buffer_dirty(ibh);
}

static int ramfsv2_drop_inode(struct inode *inode)
{
	return generic_drop_inode(inode);
}

const struct super_operations ramfs_ops = {
    .alloc_inode    = ramfsv2_alloc_inode,
    .destroy_inode  = ramfsv2_destroy_inode,
    .write_inode    = ramfsv2_write_inode,
    .dirty_inode    = ramfsv2_dirty_inode,
    .drop_inode = ramfsv2_drop_inode,
#if 0
    .evict_inode    = ramfsv2_evict_inode,
    .put_super  = ramfsv2_put_super,
    .sync_fs    = ramfsv2_sync_fs,
    .freeze_fs  = ramfsv2_freeze,
    .unfreeze_fs    = ramfsv2_unfreeze,
    .statfs     = ramfsv2_statfs,
    .remount_fs = ramfsv2_remount,
    .show_options   = ramfsv2_show_options,
    .bdev_try_to_free_page = bdev_try_to_free_page,
#endif
#if 0
	.statfs		= simple_statfs,
	.drop_inode	= generic_delete_inode,
#endif
	.show_options	= ramfs_show_options,
};

int ramfsv2_supper_init(void)
{
	return 0;
}
