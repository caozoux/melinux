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
#include <linux/hugetlb.h>
#include <asm/current.h>
#include <asm/futex.h>
#include <linux/syscalls.h>
#include <linux/miscdevice.h>
#include <linux/hugetlb.h>
#include "page_inif.h"

static long icmp_hook_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static void test_code(struct vm_area_struct *vma, unsigned long addr);


static const struct file_operations icmp_hook_ops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = icmp_hook_unlocked_ioctl,
};

static struct miscdevice icmp_hook_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "vma_dump",
	.fops = &icmp_hook_ops,
};

static long icmp_hook_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	short unsigned int id_val;
	unsigned long address;
	struct vm_area_struct *vma;
	int ret = 0;
	switch (cmd) {
		case 1:
			address = arg;
			vma = find_vma(current->mm, address);
			printk("zz %s vma:%lx \n",__func__, (unsigned long)vma);
			test_code(vma,address);
#if 0
			if (copy_from_user(&address, (char __user *)arg, 4)) {
					printk("copy to user failed\n");
					ret = -EFAULT;
					goto OUT;
			}
#endif
			break;
		default:
			ret = -EINVAL;
	}
OUT:
	return ret;
}

static void test_code(struct vm_area_struct *vma, unsigned long addr)
{
	unsigned long val;
	struct page *page;
	struct mm_struct *mm = vma->vm_mm;
	//struct mm_struct *mm = current->mm;
	struct hstate *h = hstate_vma(vma);
	unsigned long sz = huge_page_size(h);
	unsigned long mmun_start;
	pte_t *src_pte;

	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd = NULL;
	pte_t *pte = NULL;
	if (unlikely(is_vm_hugetlb_page(vma)))
		printk("zz %s %d hugetlb vm\n", __func__, __LINE__);
	for (addr = vma->vm_start; addr < vma->vm_end; addr += sz) {
		printk("zz %s addr:%lx \n",__func__, (unsigned long)addr);
		pgd = pgd_offset(mm, addr);
		if (pgd_present(*pgd)) {
			pud = pud_offset(pgd, addr);
			if (pud_present(*pud)) {
				if (pud_large(*pud)) {
					printk("zz %s %d pud_large\n", __func__, __LINE__);
					pte= (pte_t *)pud;
				}
				pmd = pmd_offset(pud, addr);
				pte = (pte_t *)pmd;
				printk("zz %s pte:%lx *pte:%lx \n",__func__, (unsigned long)pte, *pte);
				val=*(unsigned long*)pte;
				val=phys_to_virt(val&~0xffff000000000fff);
				page=virt_to_page(val);
				if (page)
					printk("zz %s page:%lx val:%lx\n",__func__, (unsigned long)page, atomic_read(&page->_mapcount));
				if (PageHuge(page)) {
					printk("zz %s %d pud_large\n", __func__, __LINE__);
				}
			} else {
				continue;
			}
		}
	}
}

void vma_dump_init(void)
{
#if 0
	unsigned long addr;
	struct vm_area_struct *vma;

	struct rb_node *rb_node;
	rb_node = current->mm->mm_rb.rb_node;

	while (rb_node) {
		 struct vm_area_struct *vma_tmp;
		 vma_tmp = rb_entry(rb_node,
				 struct vm_area_struct, vm_rb);
		 printk("zz %s vma_tmp:%lx \n",__func__, (unsigned long)vma_tmp);
		 test_code(vma_tmp, vma_tmp->vm_start);
		 rb_node = rb_node->rb_right;
	}

	//vma = find_vma(current->mm, addr);
	//printk("zz %s vma:%lx \n",__func__, (unsigned long)vma);
	//vfree(addr);
#endif
	if (misc_register(&icmp_hook_dev)) {
		pr_err("misc register err\n");
		return -ENODEV;
	}
}


void vma_dump_exit(void)
{
	misc_deregister(&icmp_hook_dev);
}
