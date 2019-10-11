/* file-mmu.c: ramfs MMU-based file operations
 *
 * Resizable simple ram filesystem for Linux.
 *
 * Copyright (C) 2000 Linus Torvalds.
 *               2000 Transmeta Corp.
 *
 * Usage limits added by David Gibson, Linuxcare Australia.
 * This file is released under the GPL.
 */

/*
 * NOTE! This filesystem is probably most useful
 * not as a real filesystem, but as an example of
 * how virtual filesystems can be written.
 *
 * It doesn't get much simpler than this. Consider
 * that this file implements the full semantics of
 * a POSIX-compliant read-write filesystem.
 *
 * Note in particular how the filesystem does not
 * need to implement any data structures of its own
 * to keep track of the virtual data: using the VFS
 * caches is sufficient.
 */

#include <linux/fs.h>
#include <linux/mm.h>

#include "internal.h"

int __set_page_dirty_no_writeback(struct page *page)
{
	printk("zz %s %d \n", __func__, __LINE__);
	if (!PageDirty(page))
		return !TestSetPageDirty(page);
	return 0;
}

static int local_simple_write_begin(struct file *file, struct address_space *mapping,
            loff_t pos, unsigned len, unsigned flags,
            struct page **pagep, void **fsdata)
{
	void **slot;
	struct radix_tree_iter iter;
	printk("zz %s pos:%lx len:%lx flags:%lx \n",__func__, (unsigned long)pos, (unsigned long)len, (unsigned long)flags);
	radix_tree_for_each_slot(slot, &mapping->page_tree, &iter, 0) {
		struct page *page;

        page = radix_tree_deref_slot(slot);
        if (unlikely(!page))
            continue;

		printk("zz %s page:%lx \n",__func__, (unsigned long)page);
	}

	return simple_write_begin(file, mapping, pos, len, flags, pagep, fsdata);
}

const struct address_space_operations ramfs_aopsv2 = {
	.readpage	= simple_readpage,
	.write_begin	= local_simple_write_begin,
	.write_end	= simple_write_end,
	.set_page_dirty = __set_page_dirty_no_writeback,
};

const struct file_operations ramfs_file_operationsv2 = {
	.read		= do_sync_read,
	.aio_read	= generic_file_aio_read,
	.write		= do_sync_write,
	.aio_write	= generic_file_aio_write,
	.mmap		= generic_file_mmap,
	.fsync		= noop_fsync,
	.splice_read	= generic_file_splice_read,
	.splice_write	= generic_file_splice_write,
	.llseek		= generic_file_llseek,
};

const struct inode_operations ramfs_file_inode_operationsv2 = {
	.setattr	= simple_setattr,
	.getattr	= simple_getattr,
};
