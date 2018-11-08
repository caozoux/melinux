#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/mmzone.h>
#include <linux/init.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <asm/current.h>
#include <asm/futex.h>
#include <linux/syscalls.h>
#include <linux/hugetlb.h>
#include "page_inif.h"


void scan_all_page(void)
{
       struct task_struct *task;
       struct mm_struct *mm;
       struct vm_area_struct * vma;
       unsigned long sz = 0x200000;
	   unsigned long addr, val, phy;
	   pgd_t *pgd;
	   pud_t *pud;
	   pmd_t *pmd = NULL;
	   pte_t *pte = NULL;

       for_each_process(task) {
			struct rb_node *rb_node;
			mm =task->mm;
			if (mm) {
				rb_node = mm->mm_rb.rb_node;
				trace_printk("zz %s task->comm:%s pid:%d \n",__func__, task->comm, task->pid);
				while (rb_node) {
					struct vm_area_struct *vma_tmp;
					struct page *page;

					vma_tmp = rb_entry(rb_node,
							   struct vm_area_struct, vm_rb);
					if (!vma_tmp)
						continue;

					//printk("zz %s vma_tmp->vm_start:%lx vma_tmp->vm_end:%lx \n",__func__, vma_tmp->vm_start, vma_tmp->vm_end);
					for (addr = vma_tmp->vm_start; addr < vma_tmp->vm_end; addr += sz) {
						page = virt_to_page(addr);
						pgd = pgd_offset(mm, addr);
						if (pgd_present(*pgd)) {
							pud = pud_offset(pgd, addr);
							if (pud_present(*pud)) {
								if (pud_large(*pud)) {
									trace_printk("zz find huge\n");
								}
								pmd = pmd_offset(pud, addr);
								if (pmd_present(*pmd)) {
									pte = (pte_t *) pmd;
									if (pte_huge(*pte))
										trace_printk("zz pte huge\n");
#if 0
									val=*(unsigned long*)pte;
									phy = (val&(~0xffff000000000fff));
									page = virt_to_page(phys_to_virt(phy));
									if (PageHuge(page))
										trace_printk("zz page huge, mapcout:%d\n", atomic_read(&page->_mapcount));
#endif

									pte = pte_offset_kernel(pmd, addr);
									if (pte_present(*pte)) {
										//val=*(unsigned long*)pte;
										//phy = (val&(~0xffff000000000fff));
										//page = virt_to_page(phys_to_virt(phy));
										//printk("zz %s val:%lx page:%lx \n",__func__, (unsigned long)val, (unsigned long)page);
										//if (PageHuge(page))
										//	printk("zz %s %d huge\n", __func__, __LINE__);
										//printk("zz %s %d pte exit\n", __func__, __LINE__);
									}
								}
							}
						}

					}

					rb_node = rb_node->rb_right;
				}

			}
	   }

}
