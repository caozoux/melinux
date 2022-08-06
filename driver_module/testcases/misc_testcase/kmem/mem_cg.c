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
#include <linux/version.h>
#if LINUX_VERSION_CODE >  KERNEL_VERSION(5,0,0)
#include <linux/memcontrol.h>
#endif
//#include <asm/tlb.h>

#include "../template_iocmd.h"
#include "../misc_ioctl.h"
#include "../debug_ctrl.h"
#include "mekernel.h"
#include "kmemlocal.h"

#define for_each_mem_cgroup_tree(iter, root) \
	for (iter = orig_mem_cgroup_iter(root, NULL, NULL);  \
			iter != NULL;  \
			iter = orig_mem_cgroup_iter(root, iter, NULL))

#define for_each_mem_cgroup(iter) \
	for (iter = orig_mem_cgroup_iter(NULL, NULL, NULL);  \
			iter != NULL;  \
			iter = orig_mem_cgroup_iter(NULL, iter, NULL))



unsigned long (*orig_node_page_state)(struct pglist_data *pgdat,
                          enum node_stat_item item);

struct mem_cgroup *(*orig_mem_cgroup_iter)(struct mem_cgroup *root, struct mem_cgroup *prev, struct mem_cgroup_reclaim_cookie *reclaim);
struct mem_cgroup *orig_root_mem_cgroup; 
unsigned long (*orig_try_to_free_mem_cgroup_pages)(struct mem_cgroup *memcg, unsigned long nr_pages,gfp_t gfp_mask, bool may_swap);

static inline struct lruvec *local_mem_cgroup_lruvec(struct mem_cgroup *memcg,
                           struct pglist_data *pgdat)
{
#if LINUX_VERSION_CODE <  KERNEL_VERSION(5,0,0)
    struct mem_cgroup_per_node *mz; 
    struct lruvec *lruvec;

    if (mem_cgroup_disabled()) {
        lruvec = &pgdat->__lruvec;
        goto out; 
    } 

    if (!memcg)
        memcg = orig_root_mem_cgroup;

    mz = mem_cgroup_nodeinfo(memcg, pgdat->node_id);
    lruvec = &mz->lruvec;

out:
    /*   
     * Since a node can be onlined after the mem_cgroup was created,
     * we have to be prepared to initialize lruvec->pgdat here;
     * and if offlined then reonlined, we need to reinitialize it.
     */

    if (unlikely(lruvec->pgdat != pgdat))
        lruvec->pgdat = pgdat;

    return lruvec;
#else
    return NULL;
#endif
}

//LRU_INACTIVE_ANON
//LRU_ACTIVE_ANON
//LRU_INACTIVE_FILE
//LRU_ACTIVE_FILE
//LRU_UNEVICTABLE

int scan_lur(struct lruvec *lruvec, int zid, enum lru_list lru)
{
	struct list_head *start,  *src = &lruvec->lists[lru];
	unsigned long size = 0;
	int num = 0;
	int age;

	start = src;
	do {
		//struct address_space *mapping;
		struct page *page;
		//pte_t *ptep, entry;
		//ptep = get_pte(addr, struct mm_struct *mm)
		page = lru_to_page(src);

		get_page_rmap_addr(page);
		//page_info_show(page);
		src = src->next;
#ifdef KIDLED_AGE_NOT_IN_PAGE_FLAGS
		age = ((page->flags >> KIDLED_AGE_PGSHIFT) & KIDLED_AGE_MASK);
		//printk("zz %s age:%lx \n",__func__, (unsigned long)age);
#endif
		num++;
	} while(src != start);
	printk("zz %s num:%ld \n",__func__, (unsigned long)num);

#if 0
	while (!list_empty(src)) {
		struct address_space *mapping;
		struct page *page;
		page = lru_to_page(src);

		src = src->next;
	}
#endif
	//for_each_evictable_lru(lru) {}
	size = mem_cgroup_get_zone_lru_size(lruvec, lru, zid);
	printk("memcg zid:%ld size:%ld \n", (unsigned long)zid, (unsigned long)size);
	return num;
}

static void dump_memcg_cgroup(struct cgroup *cg)
{
	//printk("group sysfs name: %s\n", cg->kn->name);
}

static void dump_memcg_root_cgroup(struct cgroup_root *cg)
{
	//printk("root name:%s\n", cg->name);
}

static void dump_memcg_subsys(struct cgroup_subsys  *subsys)
{
	//printk("legacy_name:%s\n", subsys->legacy_name);
	dump_memcg_root_cgroup(subsys->root);
}

static void dump_memcg_css(struct cgroup_subsys_state *css)
{
	//percpu_ref_put(&css->refcnt);
#if LINUX_VERSION_CODE <  KERNEL_VERSION(5,0,0)
	printk("ref:%lx\n", atomic_long_read(&css->refcnt.count));
#endif
	dump_memcg_cgroup(css->cgroup);
	dump_memcg_subsys(css->ss);
}

static void dump_memcg_info(struct mem_cgroup *memcg)
{
#if LINUX_VERSION_CODE <  KERNEL_VERSION(5,0,0)
	printk("cgroup name:%s\n", memcg->css.cgroup->kn->name);
	//name = cfile.file->f_path.dentry->d_name.name;
	printk("memory counts:%ld\n",page_counter_read(&memcg->memory));
	printk("swap counts:%ld\n",page_counter_read(&memcg->swap));
	printk("memsw counts:%ld\n",page_counter_read(&memcg->memsw));
	printk("kmem counts:%ld\n",page_counter_read(&memcg->kmem));
	dump_memcg_css(&memcg->css);
#endif
	//memory swap memsw kmem
}

static void scan_root_memcg(void)
{
#if LINUX_VERSION_CODE <  KERNEL_VERSION(5,0,0)
	struct mem_cgroup *iter;
	struct cgroup_subsys_state *css;
	int size;
	for_each_mem_cgroup(iter) {
		css = &iter->css;
		if (!strcmp("test", css->cgroup->kn->name)) {
			pg_data_t *pgdat = first_online_pgdat();
			struct lruvec *lruvec = local_mem_cgroup_lruvec(iter, pgdat);
			if (!lruvec)
				break;
			dump_memcg_info(iter);
			size = scan_lur(lruvec, ZONE_DMA32, LRU_INACTIVE_FILE);
			printk("zz %s size:%lx LRU_INACTIVE_FILE\n",__func__, (unsigned long)size);
			size = scan_lur(lruvec, ZONE_DMA32, LRU_ACTIVE_FILE);
			printk("zz %s size:%lx LRU_ACTIVE_FILE\n",__func__, (unsigned long)size);
			size = scan_lur(lruvec, ZONE_DMA32, LRU_ACTIVE_ANON);
			printk("zz %s size:%lx LRU_ACTIVE_ANON\n",__func__, (unsigned long)size);
			size = scan_lur(lruvec, ZONE_DMA32, LRU_INACTIVE_ANON);
			printk("zz %s size:%lx LRU_INACTIVE_ANON\n",__func__, (unsigned long)size);
		}
		//orig_try_to_free_mem_cgroup_pages(iter, 12,GFP_KERNEL,false);
	}
#endif
}

int enumerate_node_memcg(void)
{
	pg_data_t *pgdat;
	//unsigned long start_pfn, end_pfn;
	//unsigned long pfn;
	struct mem_cgroup *target_memcg = NULL, *memcg;
	unsigned long file_inactive, file_active, file_isolated;
	unsigned long anon_inactive, anon_active, anon_isolated;

	for_each_online_pgdat(pgdat) {
		file_inactive = orig_node_page_state(pgdat, NR_INACTIVE_FILE);
		file_active = orig_node_page_state(pgdat, NR_ACTIVE_FILE);
		file_isolated = orig_node_page_state(pgdat, NR_ISOLATED_FILE);
		anon_inactive = orig_node_page_state(pgdat, NR_INACTIVE_ANON);
		anon_active = orig_node_page_state(pgdat, NR_ACTIVE_ANON);
		anon_isolated = orig_node_page_state(pgdat, NR_ISOLATED_ANON);

		printk("file_inactive:%ld file_active:%ld file_isolated:%ld \n",
				(unsigned long)file_inactive, (unsigned long)file_active, (unsigned long)file_isolated);

		printk("anon_inactive:%ld anon_active:%ld anon_isolated:%ld \n",
				(unsigned long)anon_inactive, (unsigned long)anon_active, (unsigned long)anon_isolated);

		memcg=target_memcg=orig_mem_cgroup_iter(target_memcg, NULL, NULL);
		do {
			//struct lruvec *lruvec = local_mem_cgroup_lruvec(memcg, pgdat);
			//scan_lur(lruvec);
		} while ((memcg = orig_mem_cgroup_iter(target_memcg, memcg, NULL)));
	}
	scan_root_memcg();
	return 0;
}

