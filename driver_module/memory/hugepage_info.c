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
#include "page_inif.h"

spinlock_t *orig_hugetlb_lock;
void dump_hugepage_activate(void)
{
	struct hstate *hstates;
	struct hstate *cur_hstates;
	int i;
	hstates = (struct hstate*)kallsyms_lookup_name("hstates");

	if (!hstates) {
		printk("find sysmbol hstates failed \n");
		return;	
	}
	orig_hugetlb_lock= (struct hstate*)kallsyms_lookup_name("hugetlb_lock");
	if (!orig_hugetlb_lock) {
		printk("find sysmbol hugetlb_lockfailed \n");
		return;	
	}
	for (i = 0; i < HUGE_MAX_HSTATE; ++i) {
		cur_hstates = hstates + i;
		if (cur_hstates) {
			struct page *page, *next;
			printk("zz %s cur_hstates->nr_huge_pages:%08x cur_hstates->free_huge_pages:%08x \n",__func__, (int)cur_hstates->nr_huge_pages, (int)cur_hstates->free_huge_pages);
			spin_lock(orig_hugetlb_lock);
			if (cur_hstates->hugepage_activelist.next) {
				list_for_each_entry_safe(page, next, &cur_hstates->hugepage_activelist, lru) {
					int cnt;
					cnt = atomic_read(&page->_mapcount);
					if (cnt >=1) {
						printk("zz %s page:%lx mapcount:%d\n",__func__, (unsigned long)page, cnt);
						printk("zz %s page->mapping:%lx\n",__func__, (unsigned long)page->mapping);
					}
				}
			}
			spin_unlock(orig_hugetlb_lock);
			//page = list_entry(h->hugepage_freelists[nid].next, struct page, lru);])
		}
	}
	printk("zz %s hstates:%lx \n",__func__, (unsigned long)hstates);
}

