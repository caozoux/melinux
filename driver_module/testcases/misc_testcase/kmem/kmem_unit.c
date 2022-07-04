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
#include <linux/version.h>
#include <linux/mmu_notifier.h>
//#include <asm/tlb.h>

#include "../template_iocmd.h"
#include "../block/blocklocal.h"
#include "../misc_ioctl.h"
#include "../debug_ctrl.h"
#include "mekernel.h"
#include "kmemlocal.h"



extern struct hstate (*orig_hstates);
extern int (*orig_hugetlb_max_hstate);

extern void (*orig_flush_tlb_all)(void);
extern unsigned long (*orig_try_to_free_mem_cgroup_pages)(struct mem_cgroup *memcg, unsigned long nr_pages,gfp_t gfp_mask, bool may_swap);
extern unsigned long (*orig_isolate_migratepages_block)(struct compact_control *cc, unsigned long low_pfn,
		unsigned long end_pfn, isolate_mode_t isolate_mode);
extern struct mem_cgroup *(*orig_mem_cgroup_iter)(struct mem_cgroup *root, struct mem_cgroup *prev, struct mem_cgroup_reclaim_cookie *reclaim);
extern struct mem_cgroup *orig_root_mem_cgroup; 
extern unsigned long (*orig_node_page_state)(struct pglist_data *pgdat,
                          enum node_stat_item item);
struct page * (*orig___alloc_pages_direct_compact)(gfp_t gfp_mask, unsigned int order,
	  unsigned int alloc_flags, const struct alloc_context *ac,
	  enum compact_priority prio, int *compact_result);

void (*orig_flush_tlb_all)(void);
//int (*orig_zap_huge_pud)(struct mmu_gather *tlb, struct vm_area_struct *vma, pud_t *pud, unsigned long addr);
struct page *(*orig__vm_normal_page)(struct vm_area_struct *vma, unsigned long addr, pte_t pte, bool with_public_device);
void (*orig_arch_tlb_gather_mmu)(struct mmu_gather *tlb, struct mm_struct *mm, unsigned long start, unsigned long end);
int (*orig_walk_page_vma)(struct vm_area_struct *vma, struct mm_walk *walk);
int (*orig_swp_swapcount)(swp_entry_t entry);
spinlock_t *(*orig___pmd_trans_huge_lock)(pmd_t *pmd, struct vm_area_struct *vma);
struct page *(*orig_follow_trans_huge_pmd)(struct vm_area_struct *vma,
                   unsigned long addr,
                   pmd_t *pmd,
                   unsigned int flags);
void (*orig_page_idle_clear_pte_refs)(struct page *page);

void pmd_clear_bad(pmd_t *pmd)
{
    pmd_ERROR(*pmd);
    pmd_clear(pmd);
}

static void dump_kernel_page_attr(struct ioctl_data *data, u64 start_pfn, u64 size)
{
	struct page *page;
	int i;

	for (i = 0; i < size; ++i) {
		page = pfn_to_page(start_pfn + i);

		if (PageCompound(page))
			page = compound_head(page);

		page_info_show(page);
		
		printk("pfn:%llx flags:%lx recount:%d mapcout:%d mapping:%llx compound:%d isBuddy:%d isCompad:%d isLRU:%d moveable:%d order:%ld\n"
				, start_pfn + i
				, page->flags
				, page_count(page)
				, page_mapcount(page)
				, (u64)page_mapping(page)
				, PageCompound(page)
				, PageBuddy(page)
				, compound_order(page)
				, PageLRU(page)
				, __PageMovable(page)
				, PageBuddy(page) ? page_private(page) : compound_order(page)
				//, PageBuddy(page) ? page_private(page) : compound_order(page)
				);
	}
}

//#define __START_KERNEL_map_test  (0xffffffff8000000UL)
#define __START_KERNEL_map_test  _AC(0xffffffff80000000, UL)

static inline unsigned long __mephys_addr_nodebug(unsigned long x)
{
	 unsigned long y = x - __START_KERNEL_map;
	 printk("%lx\n", x - __START_KERNEL_map_test);
	 printk("zz %s x:%lx y:%lx %lx\n",__func__, (unsigned long)x, (unsigned long)y, __START_KERNEL_map);
	 printk("zz %s __START_KERNEL_map:%lx phys_base:%lx PAGE_OFFSET:%lx page_offset_base:%lx vmemmap_base::%lx \n",__func__, (unsigned long)__START_KERNEL_map, (unsigned long)phys_base, (unsigned long)PAGE_OFFSET, (unsigned long)page_offset_base, vmemmap_base);
	 printk("zz %s a:%lx b:%lx c:%lx  %lx %lx\n",__func__, x, x - __START_KERNEL_map, y + phys_base, __START_KERNEL_map - PAGE_OFFSET, y + __START_KERNEL_map - PAGE_OFFSET);
	 x = y + ((x > y) ? phys_base : (__START_KERNEL_map - PAGE_OFFSET));

	 return x;
}

static void buddy_test(void)
{
	void *buf;
	struct page *page;
	int i;

	buf = kmalloc(PAGE_SIZE, GFP_KERNEL);
	page = virt_to_page(buf);

	printk("show kmalloc order 0\n");
	page_info_show(page);
	kfree(buf);
	printk("show kfree order 0\n");
	page_info_show(page);

	buf = kmalloc(PAGE_SIZE*4, GFP_KERNEL);
	page = virt_to_page(buf);

	printk("show kmalloc order 2\n");
	get_page(page);
	for (i = 0; i < 4; ++i) {
		page_info_show(page + i);
	}
	kfree(buf);
	printk("show kfree order 2\n");
	for (i = 0; i < 4; ++i)
		page_info_show(page + i);

	printk("show alloce page order 2\n");
	//page = alloc_pages(__GFP_MOVABLE | GFP_HIGHUSER | __GFP_ZERO | __GFP_THISNODE | __GFP_NORETRY, 2);
	page = alloc_pages(__GFP_MOVABLE | GFP_HIGHUSER | __GFP_ZERO | __GFP_THISNODE | __GFP_NORETRY, 2);
	for (i = 0; i < 4; ++i) {
		page_info_show(page + i);
	}
	__free_pages(page, 2);
	printk("show free page order 2\n");
	for (i = 0; i < 4; ++i) {
		page_info_show(page + i);
	}
}

static void page_test(void)
{
	struct zone *zone;
	void *buf;
	u64 pfn;
	struct page *page;
	struct per_cpu_pages *pcp;

	printk("test kmalloc 16K start\n");
	buf = kmalloc(PAGE_SIZE * 4 , GFP_KERNEL);
	page = virt_to_page(buf);
	zone = page_zone(page);
	pcp = &this_cpu_ptr(zone->pageset)->pcp;
	pfn = page_to_pfn(page);
	page_info_show(page);
	kfree(buf);
	printk("test kmalloc 16K end\n");

	printk("test alloc_pages_exact 16K start +\n");
	buf = alloc_pages_exact(PAGE_SIZE * 4, __GFP_ZERO);
	page = virt_to_page(buf);
	page_info_show(page);
	printk("test alloc_pages_exact 16K end -\n");
}

/* full page scan will scan the whole of node pgdat page attr
 * we will use pgdata_list to scan for supporting numa machine,
 * uma machine onyl one pgdata
 */
static void full_page_scan(void)
{
	pg_data_t *pgdat;
	unsigned long start_pfn, end_pfn;
	unsigned long pfn;

	unsigned long anon_page;

	for_each_online_pgdat(pgdat) {
		end_pfn = pgdat_end_pfn(pgdat);
		start_pfn = pgdat->node_start_pfn;
		pfn = start_pfn;
		printk("zz %s start_pfn:%08x end_pfn:%08x \n",__func__, (int)start_pfn, (int)end_pfn);
		while(pfn < end_pfn) {
			struct page *page = pfn_to_page(pfn);
			if (PageAnon(page))
				anon_page++;
			if (PageAnon(page))
				anon_page++;
			pfn++;
		}
	}
}

int kmem_unit_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data)
{
	int ret = -1;
	pte_t *ptep, pte;
	unsigned long pfn;
	struct page *page;

	printk("zz %s %d cmdcode:%d\n", __func__, __LINE__, data->cmdcode);

	switch (data->cmdcode) {
		case  IOCTL_USEKMEM_SHOW:
			MEDEBUG("mem_readlock_test_start\n");
			kmem_dump_state();
			break;

		case IOCTL_USEKMEM_VMA_SCAN:
			printk("zz %s %d \n", __func__, __LINE__);
			MEDEBUG("mem vma scan\n");
			vma_scan(data);
			break;

		case IOCTL_USEKMEM_GET_PTE:
			MEDEBUG("mem pte get\n");
			ptep = get_pte(data->kmem_data.addr, current->mm);
			data->kmem_data.val = *((u64*) ptep);
			dump_pte_info((unsigned long)ptep);
			break;

		case IOCTL_USERCU_READTEST_END:
			MEDEBUG("mem_readlock_test_stop\n");
			//mem_readlock_test_stop();
			break;

		case IOCTL_USEKMEM_PAGE_ATTR:
			MEDEBUG("kmem page attr\n");
			dump_kernel_page_attr(data, data->kmem_data.pageattr_data.start_pfn,
					data->kmem_data.pageattr_data.size);
			break;

		case IOCTL_USEKMEM_TESTBUDDY:
			MEDEBUG("kmem buddy page\n");
			buddy_test();
			page_test();
			break;

		case IOCTL_USEKMEM_FULL_PAGE_SCAN:
			MEDEBUG("full page scan\n");
			full_page_scan();
			break;

		case IOCTL_USEKMEM_SLUB_OP:
			if (data->kmem_data.slub_ctrl.op == SLUB_OP_CREATE) {
				MEDEBUG("slub create\n");
				ret = kmem_kmemcache_create(data->kmem_data.name, data->kmem_data.slub_ctrl.slub_size);
			} else if (data->kmem_data.slub_ctrl.op == SLUB_OP_REMOVE) {
				MEDEBUG("slub remove\n");
				kmem_kmemcache_remove(data->kmem_data.name);
			} else if (data->kmem_data.slub_ctrl.op == SLUB_OP_ADD) {
				MEDEBUG("slub add\n");
				ret = kmem_kmemcache_create_objs(data->kmem_data.name, data->kmem_data.slub_ctrl.count, 0);
			} else if (data->kmem_data.slub_ctrl.op == SLUB_OP_DEC) {
				MEDEBUG("slub dec\n");
				ret = kmem_kmemcache_create_objs(data->kmem_data.name, data->kmem_data.slub_ctrl.count, 1);
			} else {
				pr_warn("kmem slub operation not support\n");
			}
			//ret = kmemcache_create(, );
			break;

		case IOCTL_USEKMEM_RESOURCE_SCAN:
			resource_scan();
			break;

		case IOCTL_USEKMEM_TESTMMAP:
			printk("zz %s addr:%lx \n",__func__, (unsigned long)data->kmem_data.addr);
			ptep = get_pte(data->kmem_data.addr, current->mm);
			printk("zz %s pte:%lx +\n",__func__, (unsigned long)pte_val(*ptep));
			pte=*ptep;
			if (ptep != NULL) {
				pfn = pte_pfn(*ptep);
				page = pfn_to_page(pfn);
				page_info_show(page);
				if (get_page_unless_zero(page)) {
					//set_page_idle(page);
					orig_page_idle_clear_pte_refs(page);
					//ptep_clear_flush_young_notify(vma, address, ptep);
					//pte = pte_mkclean(pte);
					//pte = pte_mkold(pte);
					//pte = pte_mkyoung(pte);
					put_page(page);
				}
				get_page_rmap_addr(page);
				//set_pte_at(current->mm, data->kmem_data.addr, ptep, pte);
				//flush_cache_range(current->mm, data->kmem_data.addr, data->kmem_data.addr + PAGE_SIZE);
				//mmu_notifier_clear_flush_young(current->mm, data->kmem_data.addr, data->kmem_data.addr + PAGE_SIZE);
				orig_flush_tlb_all();
			}
			printk("zz %s pte:%lx -\n",__func__, (unsigned long)pte_val(*ptep));
			//printk("zz %s pte:%lx \n",__func__, (unsigned long)pte);
			//printk("zz %s pfn:%lx page:%lx \n",__func__, (unsigned long)pfn, (unsigned long)page);
			break;
		default:
			goto OUT;
	}
OUT:
	return ret;
}

static void __maybe_unused test1(void)
{
	struct inode *inode;

	inode=get_inode_with_filename("/root/bigfile");

	printk("zz %s inode:%lx \n",__func__, (unsigned long)inode);
	scan_inode_radix_pagecache(inode);

}

int kmem_unit_init(void)
{
	LOOKUP_SYMS(vm_zone_stat);
	LOOKUP_SYMS(vm_numa_stat);
	LOOKUP_SYMS(vm_node_stat);
#if LINUX_VERSION_CODE <  KERNEL_VERSION(5,0,0)
	LOOKUP_SYMS(_vm_normal_page);
	LOOKUP_SYMS(arch_tlb_gather_mmu);
	LOOKUP_SYMS(walk_page_vma);
#endif
	LOOKUP_SYMS(swp_swapcount);
	LOOKUP_SYMS(__pmd_trans_huge_lock);
	LOOKUP_SYMS(follow_trans_huge_pmd);
	LOOKUP_SYMS(isolate_migratepages_block);
	LOOKUP_SYMS(iomem_resource);
	LOOKUP_SYMS(page_idle_clear_pte_refs);
	LOOKUP_SYMS(mem_cgroup_iter);
	LOOKUP_SYMS(root_mem_cgroup);
	LOOKUP_SYMS(node_page_state);
	LOOKUP_SYMS(try_to_free_mem_cgroup_pages);
	LOOKUP_SYMS(flush_tlb_all);
	LOOKUP_SYMS(anon_vma_interval_tree_iter_first);
	LOOKUP_SYMS(anon_vma_interval_tree_iter_next);
	LOOKUP_SYMS(PageHeadHuge);
	LOOKUP_SYMS(hstates);
	LOOKUP_SYMS(hugetlb_max_hstate);

	//start_node_scan_thread();
	//enumerate_node_memcg();
	//test1();
	//enum_hugetlb();
	return 0;
} 

int kmem_unit_exit(void)
{
	return 0;
}

