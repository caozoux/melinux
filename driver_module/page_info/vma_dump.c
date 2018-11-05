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
#include "page_inif.h"

static long icmp_hook_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg);


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
	int ret = 0;
	switch (cmd) {
		case 1:
			if (copy_from_user(&address, (char __user *)arg, 4)) {
					printk("copy to user failed\n");
					ret = -EFAULT;
					goto OUT;
			}
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
	struct mm_struct *mm = vma->vm_mm;
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd = NULL;
	pte_t *pte = NULL;
	printk("zz %s mm->pgd:%lx \n",__func__, (unsigned long)mm->pgd);
	pgd=mm->pgd;
	if (pgd_present(*pgd)) {
		pud = pud_offset(pgd, addr);
		if (pud_present(*pud)) {
			printk("zz %s %d \n", __func__, __LINE__);
		} else {
			printk("zz %s %d \n", __func__, __LINE__);
		}
	}
}

void vma_dump_init(void)
{
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
	if (misc_register(&icmp_hook_dev)) {
		pr_err("misc register err\n");
		return -ENODEV;
	}
}


void vma_dump_exit(void)
{
	misc_deregister(&icmp_hook_dev);
}
