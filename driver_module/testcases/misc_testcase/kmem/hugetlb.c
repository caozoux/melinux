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

#include <linux/hugetlb.h>
#include "../template_iocmd.h"
#include "../misc_ioctl.h"
#include "../debug_ctrl.h"
#include "mekernel.h"
#include "kmemlocal.h"

struct hstate (*orig_hstates);
int (*orig_hugetlb_max_hstate);

#define for_each_orig_hstate(h)   \
	for ((h) = orig_hstates; (h) < &orig_hstates[*orig_hugetlb_max_hstate]; (h)++)

static void dump_hstate_info(struct hstate *h)
{
	struct page *page, *next;
	struct list_head *activa;
	int node;

	//struct page *page = ist_entry(h->hugepage_freelists[node].next,struct page, lru); 
	printk("order:%x  nr_huge_pages:%lx\n", h->order, h->nr_huge_pages);

	if (h->nr_huge_pages) {
		printk("free_huge_pages:%lx\n", h->free_huge_pages);
		printk("resv_huge_pages:%lx\n", h->resv_huge_pages);
		printk("resv_huge_pages:%lx\n", h->resv_huge_pages);
		printk("surplus_huge_pages:%lx\n", h->surplus_huge_pages);
		for_each_node(node) {
			struct list_head *freel = &h->hugepage_freelists[node];
			list_for_each_entry_safe(page, next, freel, lru) {
				printk("free hugepage:%lx \n",(unsigned long)page);
			}
		}

		activa = &h->hugepage_activelist;
		list_for_each_entry_safe(page, next, activa, lru) {
			printk("active hugepage:%lx \n",(unsigned long)page);
			get_page_rmap_addr(page);
		}
	}
}

void enum_hugetlb(void)
{
	struct hstate *h;
	unsigned int minimum_order;

	for_each_orig_hstate(h) {
		minimum_order = huge_page_order(h);
		dump_hstate_info(h);
		//printk("zz %s minimum_order:%lx \n",__func__, (unsigned long)minimum_order);
	}
}

