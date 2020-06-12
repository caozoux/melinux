#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/of.h>
#include <linux/reboot.h>
#include <linux/uaccess.h>
#include <linux/notifier.h>
#include <linux/interrupt.h>

#include "misc_ioctl.h"
#include "template_iocmd.h"

static pte_t *pgd_pud_pnd_pte_dump(unsigned long addr)
{
	struct mm_struct *mm;
	unsigned long *p_buf;
	struct page *page;
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd = NULL;
	pte_t *pte = NULL;

	mm = current->mm;

	page=virt_to_page(addr);
	pgd = pgd_offset(mm, addr);
	if (pgd_present(*pgd)) {
		p4d_t *p4d = p4d_offset(pgd, addr);
		pud = pud_offset(p4d, addr);
		if (pud_present(*pud)) {
			if (pud_large(*pud))
				return (pte_t *)pud;
			pmd = pmd_offset(pud, addr);

			if (pmd_present(*pmd)) {
				pte = pte_offset_kernel(pmd, addr);
				if (pte_present(*pte)) {
					printk("addr:%lx page:%lx \n", addr, (unsigned long)page);
					printk("pgd:%lx pud:%lx pmd:%lx pte:%lx \n",(unsigned long)pgd, (unsigned long)pud, (unsigned long)pmd, (unsigned long)pte);
					printk("pgd:%lx pud:%lx pmd:%lx pte:%lx \n", (unsigned long)pgd_index(addr), (unsigned long)pud_index(addr), (unsigned long)pmd_index(addr), (unsigned long)pte_index(addr));
					p_buf= (unsigned long*)pgd;
					printk("pgd:%lx \n", (unsigned long)*p_buf);
					p_buf= (unsigned long*)pud;
					printk("pud:%lx \n", (unsigned long)*p_buf);
					p_buf= (unsigned long*)pmd;
					printk("pmd:%lx \n", (unsigned long)*p_buf);

					//printk("%s pte:%lx val:%lx phy:%lx addrphy:%lx\n", (unsigned long)pte, (unsigned long)*((unsigned long *)pte),virt_to_phys((unsigned long)pte), virt_to_phys((unsigned long)addr));
					return pte;
				}
			}
		}
	}
	return pte;
}

static void test_user_page(u64 addr)
{
	struct page **pages;
	//struct page *page;
	void *data;
	u64 *p;

	p = (u64 *)addr;
	
	data =kmalloc(4096, GFP_KERNEL);
	
	pgd_pud_pnd_pte_dump((u64)data);
	printk("zz %s data:%lx page:%lx\n",__func__, (unsigned long)data, (unsigned long)virt_to_page(data));
	pages = kcalloc(1, sizeof(struct page *), GFP_KERNEL);
	get_user_pages_fast(addr, 1, 1, &pages[0]);
	printk("zz %s val:%lx \n",__func__, (unsigned long)p[0]);
}

int page_ioctl_func(unsigned int  cmd, unsigned long arg)
{
	int ret = -1;
	switch (cmd) {
		case  IOCTL_USERMAP:
			pgd_pud_pnd_pte_dump(arg);
			test_user_page((u64)arg);
			break;
		default:
			goto OUT;
	}
OUT:
	return ret;
}
