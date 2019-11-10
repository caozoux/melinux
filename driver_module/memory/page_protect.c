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
#include <asm/tlbflush.h>

static remove_protect(unsigned long addr)
{

    void **place;
    struct mm_struct *kernel_init;
    pgd_t *pgd;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    pte_t entry;

    u64 addr_aligned;

    place=(void*)addr;
    kernel_init=(struct mm_init*) 0xffff000008d449c8;
    addr_aligned =(unsigned long)place &PAGE_MASK;

    pgd = pgd_offset(kernel_init, (unsigned long)place);
    pud = pud_offset(pgd, (unsigned long)place);
    pmd = pmd_offset(pud, (unsigned long)place);
    pte = pte_offset_kernel(pmd, (unsigned long)place);


    printk("m_pgd:%lx %lx %lx %lx\n", pgd, pud, pmd, pte);
    printk("m_pgd:%lx \n", *(unsigned long*)pgd);
    printk("pmd:%lx \n", *(unsigned long*)pmd);
    printk("pud:%lx \n", *(unsigned long*)pud);
    printk("pte:%lx \n", *(unsigned long*)pte);
    entry = pte_mkwrite(*pte);
    printk("pte new:%lx \n", *(unsigned long*)&entry);
    printk("targe pte:%lx \n", *(unsigned long*)pte);
    printk("gpiodriver load \n");
    set_pte(pte, entry);
    flush_tlb_kernel_range(addr_aligned, addr_aligned+ PAGE_SIZE);

}
