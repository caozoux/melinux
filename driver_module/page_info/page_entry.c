#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/mmzone.h>
#include <linux/init.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <asm/current.h>
#include <asm/futex.h>
#include "page_inif.h"


static pte_t *pgd_pud_pnd_pte_dump(unsigned long addr)
{
	struct mm_struct *mm = current->mm;
	unsigned long *p_buf;

	struct page *page;
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd = NULL;
	pte_t *pte = NULL;

	printk("zz %s mm->pgd:%lx \n",__func__, (unsigned long)mm->pgd);
	printk("zz %s addr:%lx \n",__func__, (unsigned long)addr);
	page=virt_to_page(addr);
	pgd = pgd_offset(mm, addr);
	if (pgd_present(*pgd)) {
		pud = pud_offset(pgd, addr);
		if (pud_present(*pud)) {
			if (pud_large(*pud))
				return (pte_t *)pud;
			pmd = pmd_offset(pud, addr);

			if (pmd_present(*pmd)) {
				pte = pte_offset_kernel(pmd, addr);
				if (pte_present(*pte)) {
					printk("zz %s page:%lx \n",__func__, (unsigned long)page);
					printk("zz %s pgd:%lx pud:%lx pmd:%lx pte:%lx \n",__func__, (unsigned long)pgd, (unsigned long)pud, (unsigned long)pmd, (unsigned long)pte);
					printk("zz %s pgd:%lx pud:%lx pmd:%lx pte:%lx \n",__func__, (unsigned long)pgd_index(addr), (unsigned long)pud_index(addr), (unsigned long)pmd_index(addr), (unsigned long)pte_index(addr));
					p_buf= (unsigned long*)pgd;
					printk("zz %s pgd:%lx \n",__func__, (unsigned long)*p_buf);
					p_buf= (unsigned long*)pud;
					printk("zz %s pud:%lx \n",__func__, (unsigned long)*p_buf);
					p_buf= (unsigned long*)pmd;
					printk("zz %s pmd:%lx \n",__func__, (unsigned long)*p_buf);

					printk("zz %s pte:%lx val:%lx phy:%lx addrphy:%lx\n",__func__, (unsigned long)pte, (unsigned long)*((unsigned long *)pte),virt_to_phys(pte), virt_to_phys(addr));
					return pte;
				}
			}
		}
	}
	return pte;
}

static void dump_pgd_offset(void)
{
	printk("zz %s PGDIR_SHIFT:%lx PTRS_PER_PGD:%lx \n",__func__, (unsigned long)PGDIR_SHIFT, (unsigned long)PTRS_PER_PGD);
	printk("zz %s PUD_SHIFT:%lx PTRS_PER_PUD:%lx \n",__func__, (unsigned long)PUD_SHIFT, (unsigned long)PTRS_PER_PUD);
	printk("zz %s PMD_SHIFT:%lx PTRS_PER_PMD:%lx \n",__func__, (unsigned long)PMD_SHIFT, (unsigned long)PTRS_PER_PMD);
	printk("zz %s PAGE_SHIFT:%lx PTRS_PER_PTE:%lx \n",__func__, (unsigned long)PAGE_SHIFT, (unsigned long)PTRS_PER_PTE);
}

static int __init pagedriver_init(void)
{
	struct page *page;
	void *addr;
	unsigned long pfn;
	addr=kmalloc(4096, GFP_KERNEL);
	page=virt_to_page(addr);
	pfn=page_to_pfn(page);
	pgd_pud_pnd_pte_dump(addr);
	kfree(addr);

	addr = (void *) vmalloc((unsigned int) 4096);
	vfree(addr);

	printk("pagedriver load \n");
	//pgd_pud_pnd_pte_dump(&nr_cpu_ids);
	//scan_all_page();
	dump_hugepage_activate();
	//dump_pgd_offset();
	//vma_dump_init();
	scan_all_page();
	return 0;
}

static void __exit pagedriver_exit(void)
{

	//vma_dump_exit();
	printk("pagedriver unload \n");
}

module_init(pagedriver_init);
module_exit(pagedriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
