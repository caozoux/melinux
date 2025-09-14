#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/nodemask.h>
#include <linux/uaccess.h>
#include <ksioctl/kmem_ioctl.h>

#include "hotfix_util.h"
#include "ksysdata.h"
#include "ksysd_ioctl.h"

struct pglist_data *first_online_pgdat(void)
{
	return NODE_DATA(first_online_node);
}

struct pglist_data *next_online_pgdat(struct pglist_data *pgdat)
{
	int nid = next_online_node(pgdat->node_id);
	   
	if (nid == MAX_NUMNODES)
		return NULL;

	return NODE_DATA(nid);
}

void page_scane(void)
{
	//struct page *page;
	unsigned long pfn, end, node_end;
	pg_data_t *pgdat;
	for_each_online_pgdat(pgdat) {
		pfn = pgdat->node_start_pfn;
		node_end = pgdat_end_pfn(pgdat);
		printk("zz %s pfn:%lx node_end:%lx \n",__func__, (unsigned long)pfn, (unsigned long)node_end);
	}
}

int kmem_slab_syms_init(void)
{
	return 0;
}


int kmem_page_init(void)
{
	int ret;
	page_scane();
	//if (kmem_slab_syms_init())
	//	return -EINVAL;
	//kmem_cgroup_scan_memcg(NULL);
	

	return 0;
}

void kmem_page_exit(void)
{
}


