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
#include "kmem_local.h"

#define for_each_mem_cgroup_tree(iter, root) \
	for (iter = orig_mem_cgroup_iter(root, NULL, NULL);  \
			iter != NULL;  \
			iter = orig_mem_cgroup_iter(root, iter, NULL))

#define for_each_mem_cgroup(iter) \
	for (iter = orig_mem_cgroup_iter(NULL, NULL, NULL);  \
			iter != NULL;  \
			iter = orig_mem_cgroup_iter(NULL, iter, NULL))



struct list_head *orig_cgroup_roots;
struct cgroup_subsys **orig_cgroup_subsys;
struct mem_cgroup *orig_root_mem_cgroup;
spinlock_t *orig_css_set_lock;

unsigned long (*orig_node_page_state)(struct pglist_data *pgdat,
                          enum node_stat_item item);

struct mem_cgroup *(*orig_mem_cgroup_iter)(struct mem_cgroup *root, struct mem_cgroup *prev, struct mem_cgroup_reclaim_cookie *reclaim);
//unsigned long (*orig_try_to_free_mem_cgroup_pages)(struct mem_cgroup *memcg, unsigned long nr_pages,gfp_t gfp_mask, bool may_swap);

int kmem_cgroup_scan_memcg(struct kmem_ioctl *kmem_data)
{
	struct mem_cgroup *memcg;
	for_each_mem_cgroup(memcg) {
		struct cgroup *cgroup;
		printk("zz %s memcg:%lx \n",__func__, (unsigned long)memcg);
	}
	return 0;
}

int kmem_cgroup_syms_init(void)
{
	LOOKUP_SYMS(cgroup_roots);
	LOOKUP_SYMS(mem_cgroup_iter);
	LOOKUP_SYMS(css_set_lock);
	LOOKUP_SYMS(node_page_state);
	LOOKUP_SYMS(cgroup_roots);
	LOOKUP_SYMS(cgroup_subsys);
	return 0;
}

int kmem_cgroup_init(void)
{
	if (kmem_cgroup_syms_init())
		return -EINVAL;
	kmem_cgroup_scan_memcg(NULL);
	return 0;
}

void kmem_cgroup_exit(void)
{
	
}

