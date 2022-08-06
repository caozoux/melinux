#ifndef __ME_KMEM_VMA_H__
#define __ME_KMEM_VMA_H__
#include <linux/version.h>

extern struct anon_vma_chain *
(*orig_anon_vma_interval_tree_iter_first)(struct rb_root_cached *root,
		unsigned long first, unsigned long last);

extern struct anon_vma_chain *
(*orig_anon_vma_interval_tree_iter_next)(struct anon_vma_chain *node,
	  unsigned long first, unsigned long last);

extern int (*orig_PageHeadHuge)(struct page *page_head);
#define meanon_vma_interval_tree_foreach(avc, root, start, last)       \
		   for (avc = orig_anon_vma_interval_tree_iter_first(root, start, last); \
				   avc; avc = orig_anon_vma_interval_tree_iter_next(avc, start, last))
static inline pgoff_t orig_page_to_pgoff(struct page *page) 
{
	if (unlikely(orig_PageHeadHuge(page)))
		return page->index << compound_order(page);
	return page_to_index(page);
}

static inline unsigned long
__vma_address(struct page *page, struct vm_area_struct *vma)
{
	pgoff_t pgoff = orig_page_to_pgoff(page);
	return vma->vm_start + ((pgoff - vma->vm_pgoff) << PAGE_SHIFT);
}

static inline unsigned long
vma_address(struct page *page, struct vm_area_struct *vma)
{
	 unsigned long start, end;
	 start = __vma_address(page, vma);
#if LINUX_VERSION_CODE <  KERNEL_VERSION(5,0,0)
	 end = start + PAGE_SIZE * (hpage_nr_pages(page) - 1);
#else
	 end = start + PAGE_SIZE * (thp_nr_pages(page) - 1);
#endif
	 VM_BUG_ON_VMA(end < vma->vm_start || start >= vma->vm_end, vma);
	 return max(start, vma->vm_start);
}
#endif
