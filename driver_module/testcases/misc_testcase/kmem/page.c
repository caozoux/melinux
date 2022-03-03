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

#ifdef CONFIG_X86
int pmd_huge(pmd_t pmd)
{
    return !pmd_none(pmd) &&
        (pmd_val(pmd) & (_PAGE_PRESENT|_PAGE_PSE)) != _PAGE_PRESENT;
}

int pud_huge(pud_t pud)
{
    return !!(pud_val(pud) & _PAGE_PSE);
}
#endif

pte_t *get_pte(unsigned long addr, struct mm_struct *mm)
{
    pgd_t *pgd = pgd_offset(mm, addr);
    p4d_t *p4d = p4d_offset(pgd, addr);
    pud_t *pud = pud_offset(p4d, addr);
    pmd_t *pmd = pmd_offset(pud, addr);
    pte_t *pte;
    u64 addr_aligned;

    addr_aligned = addr & PAGE_MASK;
	pte = pte_offset_map(pmd, addr);
    //pte = pte_offset_kernel(pmd, addr);

	if (pte_none(*pte))
		return NULL;

	if (pte_present(*pte))
    	return pte;

	return NULL;
}

void vma_pte_dump(struct vm_area_struct *vma, u64 start_addr, u64 nr_page)
{
	unsigned long addr = vma->vm_start, end = addr + nr_page * PAGE_SIZE;
	struct page *page;
	u64 pfn;

    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
	pte_t  ptet;

    pgd = pgd_offset(vma->vm_mm, addr);
    p4d = p4d_offset(pgd, addr);
    pud = pud_offset(p4d, addr);

	for (addr = vma->vm_start; addr < end; addr += PAGE_SIZE) {
		if (pud_huge(*pud) || pud_none(*pud)) {
			//printk("zz %s pud:%llx \n", __func__, (u64)pmd);
			return;
			//return (pte_t*)pud;
		}

		pmd = pmd_offset(pud, addr);

		if (pmd_huge(*pmd) || pmd_none(*pmd)) {
			//printk("zz %s pdm:%llx \n", __func__, (u64)pmd);
			continue;
		}

		pte = pte_offset_map(pmd, addr);

		//if (*((u64*)pte) != 0 ) {
		//if (*((u64*)pte) != 0) {
		if (!pte_none(*pte) && *((u64*)pte) != 0) {

			pfn = pte_pfn(*pte);
			page = pfn_to_page(pfn);
			printk("addr:%llx pte:%llx %llx pageflags:%llx-%llx\n", addr, (u64)pte, *((u64*)pte), pfn, (u64)page->flags);
		}
	}
}

void page_info_show(struct page *page)
{
	struct address_space *mapping;
	enum lru_list lru;
	printk("pfn:%lld virt:%llx flag:%llx\n", page_to_pfn(page), page_to_virt(page));

	if (PageBuddy(page))
		printk("page:buddy\n");

	if (PageCompound(page))
		printk("page: Compound\n");

	if (PageAnon(page))
		printk("page: Anon\n");

	if (PageReferenced(page))
		printk("page Reference:%d\n",PageReferenced(page));

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

	printk("count:%d \n", page_count(page));
	/*
	printk("pfn:%llx virt:%llx %llx %llx buddy:%d Comp:%d LRU:%d map:%llx order:%llx count:%d head:%lx free:%lx \n",
			page_to_pfn(page), page_to_virt(page), page->page_type, page->flags, PageBuddy(page),
			PageCompound(page), PageLRU(page), page_mapping(page), page_private(page),
			page_count(page), page_to_pfn(compound_head(page)), page->index
		   );
	 */

}

