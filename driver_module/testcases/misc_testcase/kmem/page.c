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
#include <linux/rmap.h>
#include <linux/mm_inline.h>
//#include <asm/tlb.h>

#include "../template_iocmd.h"
#include "../misc_ioctl.h"
#include "../debug_ctrl.h"
#include "mekernel.h"
#include "kmemlocal.h"

int (*orig_PageHeadHuge)(struct page *page_head);

int get_page_rmap_addr(struct page *old)
{
	unsigned long address;
	struct page *head_page;
	struct anon_vma *anon_vma;
	struct anon_vma_chain *avc;
	unsigned long val;
	pte_t *ptep, entry;
	pgoff_t pgoff;
	pgoff_t pgoff_start, pgoff_end;

	if (PageAnon(old)) {
		if (PageCompound(old))
			head_page = compound_head(old);
		else
			head_page = old;

		val = (unsigned long) head_page->mapping;
		if ((val& PAGE_MAPPING_FLAGS) != PAGE_MAPPING_ANON)
			return 0;
		val &= ~PAGE_MAPPING_FLAGS;
		anon_vma = (struct anon_vma *)val;
		pgoff_start = orig_page_to_pgoff(head_page);
		pgoff_end = pgoff_start + hpage_nr_pages(head_page) - 1;
		anon_vma_interval_tree_foreach(avc, &anon_vma->rb_root,
				pgoff_start, pgoff_end) {
			int is_young = 0;
			struct vm_area_struct *vma = avc->vma;
			address = vma_address(head_page, vma);
			ptep = get_pte(address, vma->vm_mm);
			entry=*ptep;
			if (pte_young(entry)) {
				is_young = 1;
				//orig_page_idle_clear_pte_refs(head_page);
			}

			printk("zz %s address:%lx %s\n",__func__, (unsigned long)address,
										 is_young ? "is_young" : "old");
		}
	}

	return 0;
}

void page_info_show(struct page *page)
{
	struct address_space *mapping;
	enum lru_list lru;

	//printk("pfn:%lld virt:%llx flag:%llx\n", page_to_pfn(page), page_to_virt(page));
	printk("pfn:%lld  count:%d\n", page_to_pfn(page), page_count(page));

	if (PageBuddy(page))
		printk("page:buddy\n");

	if (PageCompound(page))
		printk("page: Compound\n");

	if (PageAnon(page))
		printk("page: Anon\n");

	if (PageReferenced(page))
		printk("page Reference:%d\n", PageReferenced(page));

	if (PageActive(page))
		printk("page: Active\n");
	else
		printk("page: Inactive\n");

	if (PageLRU(page)) {
		lru = page_lru(page);
		if (lru == LRU_INACTIVE_ANON)
			printk("page: LRU_INACTIVE_ANON\n");
		else if (lru == LRU_ACTIVE_ANON)
			printk("page: LRU_ACTIVE_ANON\n");
		else if (lru == LRU_INACTIVE_FILE)
			printk("page: LRU_INACTIVE_FILE\n");
		else if (lru == LRU_ACTIVE_FILE)
			printk("page: LRU_ACTIVE_FILE\n");
		else if (lru == LRU_UNEVICTABLE)
			printk("page: LRU_UNEVICTABLE\n");
		else
			printk("page: LRU ERR\n");
	}

	if (page_is_idle(page))
		printk("page: idle\n");

	if (page_is_young(page))
		printk("page: young\n");

	mapping = page_mapping(page);
	if (mapping)
		printk("page: mapping\n");


	/*
	printk("pfn:%llx virt:%llx %llx %llx buddy:%d Comp:%d LRU:%d map:%llx order:%llx count:%d head:%lx free:%lx \n",
			page_to_pfn(page), page_to_virt(page), page->page_type, page->flags, PageBuddy(page),
			PageCompound(page), PageLRU(page), page_mapping(page), page_private(page),
			page_count(page), page_to_pfn(compound_head(page)), page->index
		   );
	 */
}

