#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/pagemap.h>

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
	if(pmd_huge(*pmd))
		return (pte_t *)pmd;

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

