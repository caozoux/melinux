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

#include "../template_iocmd.h"
#include "../misc_ioctl.h"
#include "../debug_ctrl.h"
#include "mekernel.h"

struct mem_cgroup *(*orig_mem_cgroup_iter)(struct mem_cgroup *root, struct mem_cgroup *prev, struct mem_cgroup_reclaim_cookie *reclaim);
int emulater_node_memcg(void)
{
	//pg_data_t *pgdat;
	//unsigned long start_pfn, end_pfn;
	//unsigned long pfn;
	struct mem_cgroup *target_memcg =NULL, *memcg;

	target_memcg = orig_mem_cgroup_iter(target_memcg, NULL, NULL);
	do {

	} while ((memcg = orig_mem_cgroup_iter(target_memcg, memcg, NULL)));
	return 0;

}

