#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/netdevice.h>
#include <ksioctl/kblock_ioctl.h>

#include "hotfix_util.h"
#include "ksysdata.h"
#include "ksysd_ioctl.h"
#include "kblock_local.h"


struct list_head *orig_super_blocks;
spinlock_t *orig_sb_lock;

int kblock_unit_super_syminit(void)
{
	LOOKUP_SYMS(super_blocks);
	LOOKUP_SYMS(sb_lock);
	return 0;
}

void kblock_super_scan_super(void)
{
    struct super_block *sb;

    spin_lock(orig_sb_lock);
    list_for_each_entry(sb, orig_super_blocks, s_list) {
		printk("zz %s sb:%lx \n",__func__, (unsigned long)sb);
    }
    spin_unlock(orig_sb_lock);
}

void kblock_super_scan_allinode(struct super_block *sb)
{
	struct inode *inode;
	spin_lock(&sb->s_inode_list_lock);
	list_for_each_entry(inode, &sb->s_inodes, i_sb_list) {
	}
	spin_unlock(&sb->s_inode_list_lock);
}

