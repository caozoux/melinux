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
#include <linux/mpage.h>
#include <linux/writeback.h>
#include <linux/buffer_head.h>
#include <linux/kernel.h>

#include "internal.h"

int __set_page_dirty_no_writeback(struct page *page)
{
	trace_printk("\n");
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

	trace_printk("\n");
	radix_tree_for_each_slot(slot, &mapping->page_tree, &iter, 0) {
		struct page *page;

        page = radix_tree_deref_slot(slot);
        if (unlikely(!page))
            continue;

	}

	return simple_write_begin(file, mapping, pos, len, flags, pagep, fsdata);
}

int local_simple_write_end(struct file *file, struct address_space *mapping,
            loff_t pos, unsigned len, unsigned copied,
            struct page *page, void *fsdata)
{
	trace_printk("\n");
	return simple_write_end(file, mapping, pos, len, copied, page, fsdata);
}

int local_simple_readpage(struct file *file, struct page *page)
{
	trace_printk("\n");
	return simple_readpage(file, page);
}

static int local_get_blocks(struct inode *inode, sector_t iblock, struct buffer_head  *bh_result,
    			int create)
{
	trace_printk("\n");
	return 0;	
}

static int
local_readpages(struct file *unused, struct address_space *mapping, struct list_head *pages,
    			unsigned        nr_pages)
{
	trace_printk("\n");
    return mpage_readpages(mapping, pages, nr_pages, local_get_blocks);
}

static int local_writepage(struct page *page, struct writeback_control *wbc)
{
	printk("zz %s %d \n", __func__, __LINE__);
	return 0;
}

static int local_writepages(
    struct address_space    *mapping,
    struct writeback_control *wbc)
{
	trace_printk("\n");
    return generic_writepages(mapping, wbc);
}

static int xfs_vm_releasepage(struct page *page, gfp_t gfp_mask)
{
#if 0
    int      delalloc, unwritten;

    if (WARN_ON_ONCE(delalloc))
        return 0;
    if (WARN_ON_ONCE(unwritten))
        return 0;
#endif

    return try_to_free_buffers(page);
}

static void xfs_vm_invalidatepage (
    struct page     *page,
    unsigned int        offset,
    unsigned int        length)
{
    block_invalidatepage_range(page, offset, length);
}

static sector_t
xfs_vm_bmap(
    struct address_space    *mapping,
    sector_t        block)
{

	trace_printk("\n");
    return generic_block_bmap(mapping, block, local_get_blocks);
}

static ssize_t
xfs_vm_direct_IO(
    int         rw,
    struct kiocb        *iocb,
    const struct iovec  *iov,
    loff_t          offset,
    unsigned long       nr_segs)
{
	return 0;
}

#if 0
const struct address_space_operations ramfs_aopsv2 = {
	.readpage	= local_simple_readpage,
	.write_begin	= local_simple_write_begin,
	.write_end	= local_simple_write_end,
	.set_page_dirty = __set_page_dirty_no_writeback,
};
#else
const struct address_space_operations ramfs_aopsv2 = {
	.readpage	= local_simple_readpage,
	.readpages      = local_readpages,
	.writepage      = local_writepage,
	.writepages     = local_writepages,
	.set_page_dirty = __set_page_dirty_no_writeback,
	.releasepage        = xfs_vm_releasepage,
	.invalidatepage_range   = xfs_vm_invalidatepage,
	.write_begin	= local_simple_write_begin,
	.write_end	= local_simple_write_end,
	.bmap           = xfs_vm_bmap,
	.direct_IO      = xfs_vm_direct_IO,
	.migratepage        = buffer_migrate_page,
	.is_partially_uptodate  = block_is_partially_uptodate,
	.error_remove_page  = generic_error_remove_page,
};
#endif

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
