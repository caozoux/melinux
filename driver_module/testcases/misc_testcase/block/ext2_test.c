#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/of.h>
#include <linux/kthread.h>
#include <linux/reboot.h>
#include <linux/uaccess.h>
#include <linux/notifier.h>
#include <linux/interrupt.h>
#include <linux/rcupdate.h>
#include <linux/rcupdate.h>
#include <linux/delay.h>
#include <linux/kallsyms.h>
#include <linux/buffer_head.h>

#include "../template_iocmd.h"
#include "../misc_ioctl.h"
#include "../debug_ctrl.h"
#include "mekernel.h"

static struct list_head *orig_super_blocks;

struct super_block *get_block_by_name(char *name)
{
	struct super_block *sb;

	list_for_each_entry(sb, orig_super_blocks, s_list)
		if (strcmp(sb->s_id, name))
			return sb;

	return NULL;	
}

int ext2test_unit_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_data *data)
{
	struct super_block *sb;
	struct buffer_head *bh;
	int ret = -1;
	switch (data->cmdcode) {
		case IOCTL_USEEXT2_ENUM_SUPBLOCK:
			MEDEBUG("ext2test lock\n");

			list_for_each_entry(sb, orig_super_blocks, s_list)
				printk("supper_block:%s\n", sb->s_id);

			ret = 0;
			break;

		case IOCTL_USEEXT2_GET_BLOCK:
			sb = get_block_by_name(data->ext2_data.blk_name);

			if (!sb)
				goto out;

			bh = sb_bread(sb, 2);
			if (bh == NULL)
				goto out;

			printk("name:%s sb:%s\n", data->ext2_data.blk_name, sb->s_id);
			break;
	}

out:
	return ret;
}

int ext2test_unit_init(void)
{

	LOOKUP_SYMS(super_blocks);
	return 0;
}

int ext2test_unit_exit(void)
{
	//sb_bread(sb, logic_sb_block);	
	return 0;
}

