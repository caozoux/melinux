#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/swap.h>
#include <linux/swapops.h>
#include <linux/page_idle.h>
#include <linux/mm_inline.h>
//#include <asm/tlb.h>

#include "../template_iocmd.h"
#include "../misc_ioctl.h"
#include "../debug_ctrl.h"
#include "mekernel.h"

int alloce_pagecache(struct inode *inode, pgoff_t offset, unsigned long nr_to_read)
{
	/*
	LIST_HEAD(page_pool);
	gfp_t gfp_mask = readahead_gfp_mask(mapping);
	page = __page_cache_alloc(gfp_mask);
	page->index = page_offset;
	read_pages(mapping, filp, &page_pool, nr_pages, gfp_mask);
	*/
}

