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
#include <linux/sched.h>
#include <linux/sched/types.h>
//#include <asm/tlb.h>

#include "../template_iocmd.h"
#include "../misc_ioctl.h"
#include "../debug_ctrl.h"
#include "mekernel.h"

static void scan_node(pg_data_t *pgdat)
{
	unsigned long node_end, node_start, node_present;
	unsigned long pfn, nr_pages;
	unsigned long idle_nums=0, page_scan_nums=0;
	unsigned long cont_page_start =0, cont_page_end = 0; 
	struct page *page;

	node_end = pgdat_end_pfn(pgdat);
	node_start = pgdat->node_start_pfn;
	node_present = pgdat->node_present_pages;
	pfn = node_start;
	page = pfn_to_page(pfn);
	cont_page_start=cont_page_end=pfn;
	cont_page_end--;

	while(pfn < node_present) {
		if (!pfn_valid(pfn))
			goto again;
		page = pfn_to_page(pfn);
		//if (!page || !PageLRU(page))
		if (!page)
			goto again;

		if (PageTransHuge(page))
			nr_pages = 1 << compound_order(page);

		if (page_is_idle(page))
			idle_nums++;

		if (page)
			page_scan_nums++;

		if (pfn == (cont_page_end+1)) {
			cont_page_end = pfn;
		} else {
			printk("cont_page_start:%lx cont_page_end:%lx \n",(unsigned long)cont_page_start, (unsigned long)cont_page_end);
			cont_page_start=cont_page_end= pfn;
		}
again:
		pfn++;
	}
	printk("cont_page_start:%lx cont_page_end:%lx idle_nums:%ld\n",(unsigned long)cont_page_start, (unsigned long)cont_page_end, idle_nums);
}

static int kidled(void *dummy)
{
	pg_data_t *pgdat;
	 while (!kthread_should_stop()) {
		 for_each_online_pgdat(pgdat) {
			 scan_node(pgdat);
		 }
		 break;
		 //schedule_timeout_interruptible(50);
	 }
	 return 0;
}

void  start_node_scan_thread(void)
{
	struct task_struct *thread;
	struct sched_param param = { .sched_priority = 0 };

	thread = kthread_run(kidled, NULL, "kidled");
	sched_setscheduler(thread, SCHED_IDLE, &param);
}
