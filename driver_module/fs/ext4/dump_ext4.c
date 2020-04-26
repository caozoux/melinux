#include <linux/module.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/time.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/blkdev.h>
#include <linux/backing-dev.h>
#include <linux/parser.h>
#include <linux/buffer_head.h>
#include <linux/exportfs.h>
#include <linux/vfs.h>
#include <linux/random.h>
#include <linux/mount.h>
#include <linux/namei.h>
#include <linux/quotaops.h>
#include <linux/seq_file.h>
#include <linux/ctype.h>
#include <linux/log2.h>
#include <linux/crc16.h>
#include <linux/dax.h>
#include <linux/cleancache.h>
#include <linux/uaccess.h>
#include <linux/iversion.h>

#include <linux/kthread.h>
#include <linux/freezer.h>

#include "ext4.h"
#include "ext4_extents.h"	/* Needed for trace points definition */
#include "ext4_jbd2.h"
#include "xattr.h"
#include "acl.h"
#include "mballoc.h"
#include "fsmap.h"

static struct super_block *dump_sb;

void dump_init_super(struct super_block *sb)
{
	if (sb)
		dump_sb = sb;
}

void dump_group_desc(struct ext4_group_desc *desc)
{
	printk("zz %s bg_block_bitmap_lo:%lx \n",__func__, (unsigned long)bg_block_bitmap_lo);
	printk("zz %s bg_inode_bitmap_lo:%lx \n",__func__, (unsigned long)bg_inode_bitmap_lo);
	printk("zz %s bg_inode_table_lo:%lx \n",__func__, (unsigned long)bg_inode_table_lo);
	printk("zz %s bg_free_blocks_count_lo:%lx \n",__func__, (unsigned long)bg_free_blocks_count_lo);
	printk("zz %s bg_free_inodes_count_lo:%lx \n",__func__, (unsigned long)bg_free_inodes_count_lo);
	printk("zz %s bg_used_dirs_count_lo:%lx \n",__func__, (unsigned long)bg_used_dirs_count_lo);
	printk("zz %s bg_flags:%lx \n",__func__, (unsigned long)bg_flags);
	printk("zz %s bg_exclude_bitmap_lo:%lx \n",__func__, (unsigned long)bg_exclude_bitmap_lo);
	printk("zz %s bg_block_bitmap_csum_lo:%lx \n",__func__, (unsigned long)bg_block_bitmap_csum_lo);
	printk("zz %s bg_inode_bitmap_csum_lo:%lx \n",__func__, (unsigned long)bg_inode_bitmap_csum_lo);
	printk("zz %s bg_itable_unused_lo:%lx \n",__func__, (unsigned long)bg_itable_unused_lo);
	printk("zz %s bg_checksum:%lx \n",__func__, (unsigned long)bg_checksum);
	printk("zz %s bg_block_bitmap_hi:%lx \n",__func__, (unsigned long)bg_block_bitmap_hi);
	printk("zz %s bg_inode_bitmap_hi:%lx \n",__func__, (unsigned long)bg_inode_bitmap_hi);
	printk("zz %s bg_inode_table_hi:%lx \n",__func__, (unsigned long)bg_inode_table_hi);
	printk("zz %s bg_free_blocks_count_hi:%lx \n",__func__, (unsigned long)bg_free_blocks_count_hi);
	printk("zz %s bg_free_inodes_count_hi:%lx \n",__func__, (unsigned long)bg_free_inodes_count_hi);
	printk("zz %s bg_used_dirs_count_hi:%lx \n",__func__, (unsigned long)bg_used_dirs_count_hi);
	printk("zz %s bg_itable_unused_hi:%lx \n",__func__, (unsigned long)bg_itable_unused_hi);
	printk("zz %s bg_exclude_bitmap_hi:%lx \n",__func__, (unsigned long)bg_exclude_bitmap_hi);
	printk("zz %s bg_block_bitmap_csum_hi:%lx \n",__func__, (unsigned long)bg_block_bitmap_csum_hi);
	printk("zz %s bg_inode_bitmap_csum_hi:%lx \n",__func__, (unsigned long)bg_inode_bitmap_csum_hi);
	printk("zz %s bg_reserved:%lx \n",__func__, (unsigned long)bg_reserved);
}

#if 0
void dump_list_group_desc(void)
{
	int db_count;
	db_count = (sbi->s_groups_count + EXT4_DESC_PER_BLOCK(sb) - 1) /
		   EXT4_DESC_PER_BLOCK(sb);
	for (i = 0; i < db_count; i++) {
		block = descriptor_loc(sb, logical_sb_block, i);
		sb_breadahead(sb, block);
	}
}
#endif
