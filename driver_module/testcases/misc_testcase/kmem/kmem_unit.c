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
#include <asm/tlb.h>

#include "../template_iocmd.h"
#include "../misc_ioctl.h"
#include "../debug_ctrl.h"
#include "mekernel.h"

#define PSS_SHIFT 12

atomic_long_t *orig_vm_zone_stat;
atomic_long_t *orig_vm_numa_stat;
atomic_long_t *orig_vm_node_stat;
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

void pmd_clear_bad(pmd_t *pmd)
{
    pmd_ERROR(*pmd);
    pmd_clear(pmd);
}

static void __maybe_unused mem_proc_show(void)
{
	struct sysinfo i;
	si_meminfo(&i);
	si_swapinfo(&i);

	printk("MemTotal:       %ld", i.totalram);
	printk("MemFree:        %ld", i.freeram);
	printk("Buffers:        %ld", i.bufferram);
	printk("SwapTotal:      %ld", i.totalswap);
	printk("SwapFree:       %ld", i.freeswap);
	printk("Shmem:          %ld", i.sharedram);

}

#ifdef CONFIG_NUMA
static void kmem_dump_numa_item(void)
{
	printk("NUMA_HIT: %ld\n", atomic_long_read(&orig_vm_numa_stat[NUMA_HIT]));
	printk("NUMA_MISS: %ld\n", atomic_long_read(&orig_vm_numa_stat[NUMA_MISS]));
	printk("NUMA_FOREIGN: %ld\n", atomic_long_read(&orig_vm_numa_stat[NUMA_FOREIGN]));
	printk("NUMA_INTERLEAVE_HIT: %ld\n", atomic_long_read(&orig_vm_numa_stat[NUMA_INTERLEAVE_HIT]));
	printk("NUMA_LOCAL: %ld\n", atomic_long_read(&orig_vm_numa_stat[NUMA_LOCAL]));
	printk("NUMA_OTHER: %ld\n", atomic_long_read(&orig_vm_numa_stat[NUMA_OTHER]));
	printk("NR_VM_NUMA_STAT_ITEMS: %ld\n", atomic_long_read(&orig_vm_numa_stat[NR_VM_NUMA_STAT_ITEMS]));
}
#endif

static void kmem_dump_node_item(atomic_long_t *vm_stat)
{
	printk("NR_LRU_BASE: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_LRU_BASE]));
	printk("NR_INACTIVE_ANON = NR_LRU_BASE: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_INACTIVE_ANON]));
	printk("NR_ACTIVE_ANON: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_ACTIVE_ANON]));
	printk("NR_INACTIVE_FILE: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_INACTIVE_FILE]));
	printk("NR_ACTIVE_FILE: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_ACTIVE_FILE]));
	printk("NR_UNEVICTABLE: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_UNEVICTABLE]));
	printk("NR_SLAB_RECLAIMABLE: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_SLAB_RECLAIMABLE]));
	printk("NR_SLAB_UNRECLAIMABLE: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_SLAB_UNRECLAIMABLE]));
	printk("NR_ISOLATED_ANON: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_ISOLATED_ANON]));
	printk("NR_ISOLATED_FILE: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_ISOLATED_FILE]));
	printk("WORKINGSET_REFAULT: %ld\n", atomic_long_read(&orig_vm_node_stat[WORKINGSET_REFAULT]));
	printk("WORKINGSET_ACTIVATE: %ld\n", atomic_long_read(&orig_vm_node_stat[WORKINGSET_ACTIVATE]));
	printk("WORKINGSET_NODERECLAIM: %ld\n", atomic_long_read(&orig_vm_node_stat[WORKINGSET_NODERECLAIM]));
	printk("NR_ANON_MAPPED: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_ANON_MAPPED]));
	printk("NR_FILE_MAPPED: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_FILE_MAPPED]));
	printk("NR_FILE_PAGES: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_FILE_PAGES]));
	printk("NR_FILE_DIRTY: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_FILE_DIRTY]));
	printk("NR_WRITEBACK: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_WRITEBACK]));
	printk("NR_WRITEBACK_TEMP: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_WRITEBACK_TEMP]));
	printk("NR_SHMEM: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_SHMEM]));
	printk("NR_SHMEM_THPS: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_SHMEM_THPS]));
	printk("NR_SHMEM_PMDMAPPED: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_SHMEM_PMDMAPPED]));
	printk("NR_ANON_THPS: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_ANON_THPS]));
	printk("NR_VMSCAN_WRITE: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_VMSCAN_WRITE]));
	printk("NR_VMSCAN_IMMEDIATE: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_VMSCAN_IMMEDIATE]));
	printk("NR_DIRTIED: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_DIRTIED]));
	printk("NR_WRITTEN: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_WRITTEN]));
	//printk("NR_INDIRECTLY_RECLAIMABLE_BYTES: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_INDIRECTLY_RECLAIMABLE_BYTES]));
	printk("NR_VM_NODE_STAT_ITEMS: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_VM_NODE_STAT_ITEMS]));
}

static void kmem_dump_zone_item(atomic_long_t *vm_stat)
{
	//struct zone *zone;
	//for_each_populated_zone(zone)
	printk("NR_FREE_PAGES: %ld\n", atomic_long_read(&vm_stat[NR_FREE_PAGES]));
	printk("NR_vm_stat_LRU_BASE: %ld\n", atomic_long_read(&vm_stat[NR_ZONE_LRU_BASE]));
	printk("NR_vm_stat_INACTIVE_ANON: %ld\n", atomic_long_read(&vm_stat[NR_ZONE_INACTIVE_ANON]));
	printk("NR_vm_stat_ACTIVE_ANON: %ld\n", atomic_long_read(&vm_stat[NR_ZONE_ACTIVE_ANON]));
	printk("NR_vm_stat_INACTIVE_FILE: %ld\n", atomic_long_read(&vm_stat[NR_ZONE_INACTIVE_FILE]));
	printk("NR_vm_stat_ACTIVE_FILE: %ld\n", atomic_long_read(&vm_stat[NR_ZONE_ACTIVE_FILE]));
	printk("NR_vm_stat_UNEVICTABLE: %ld\n", atomic_long_read(&vm_stat[NR_ZONE_UNEVICTABLE]));
	printk("NR_vm_stat_WRITE_PENDING: %ld\n", atomic_long_read(&vm_stat[NR_ZONE_WRITE_PENDING]));
	printk("NR_MLOCK: %ld\n", atomic_long_read(&vm_stat[NR_MLOCK]));
	printk("NR_PAGETABLE: %ld\n", atomic_long_read(&vm_stat[NR_PAGETABLE]));
	printk("NR_KERNEL_STACK_KB: %ld\n", atomic_long_read(&vm_stat[NR_KERNEL_STACK_KB]));
	printk("NR_BOUNCE: %ld\n", atomic_long_read(&vm_stat[NR_BOUNCE]));
	printk("NR_ZSPAGES: %ld\n", atomic_long_read(&vm_stat[NR_ZSPAGES]));
	printk("NR_FREE_CMA_PAGES: %ld\n", atomic_long_read(&vm_stat[NR_FREE_CMA_PAGES]));
	printk("NR_VM_vm_stat_STAT_ITEMS: %ld\n", atomic_long_read(&vm_stat[NR_VM_ZONE_STAT_ITEMS]));
}

static void kmem_dump_state(void)
{
	//struct zone zone; 
	//zone.vm_stat = orig_vm_zone_stat;
	//struct zone *zone;
	//for_each_populated_zone(zone)
	kmem_dump_zone_item(orig_vm_zone_stat);
	kmem_dump_node_item(orig_vm_node_stat);
#ifdef CONFIG_NUMA
	kmem_dump_numa_item();
#endif
}

static void smaps_account(struct mem_size_stats *mss, struct page *page,
        bool compound, bool young, bool dirty, bool locked)
{
    int i, nr = compound ? 1 << compound_order(page) : 1;
    unsigned long size = nr * PAGE_SIZE;
	unsigned long phys;

	mss->page_order[compound_order(page)]++;

	phys = page_to_phys(page);
	copy_to_user(mss->page_buffer + mss->page_index, &phys, sizeof(unsigned long));

	//printk("zz %s mss->page_buffer:%lx %lx %d\n",__func__, mss->page_buffer[mss->page_index], page_to_phys(page), mss->page_index);

	mss->page_index++;

    if (PageAnon(page)) {
        mss->anonymous += size;
        if (!PageSwapBacked(page) && !dirty && !PageDirty(page))
            mss->lazyfree += size;
    }

    mss->resident += size;
    /* Accumulate the size in pages that have been accessed. */
    if (young || page_is_young(page) || PageReferenced(page))
        mss->referenced += size;

    /*
     * page_count(page) == 1 guarantees the page is mapped exactly once.
     * If any subpage of the compound page mapped with PTE it would elevate
     * page_count().
     */
    if (page_count(page) == 1) {
        if (dirty || PageDirty(page))
            mss->private_dirty += size;
        else
            mss->private_clean += size;
        mss->pss += (u64)size << PSS_SHIFT;
        if (locked)
            mss->pss_locked += (u64)size << PSS_SHIFT;
        return;
    }

    for (i = 0; i < nr; i++, page++) {
        int mapcount = page_mapcount(page);
        unsigned long pss = (PAGE_SIZE << PSS_SHIFT);

        if (mapcount >= 2) {
            if (dirty || PageDirty(page))
                mss->shared_dirty += PAGE_SIZE;
            else
                mss->shared_clean += PAGE_SIZE;
            mss->pss += pss / mapcount;
            if (locked)
                mss->pss_locked += pss / mapcount;
        } else {
            if (dirty || PageDirty(page))
                mss->private_dirty += PAGE_SIZE;
            else
                mss->private_clean += PAGE_SIZE;
            mss->pss += pss;
            if (locked)
                mss->pss_locked += pss;
        }
    }
}

static void smaps_pte_entry(pte_t *pte, unsigned long addr,
        struct mm_walk *walk)
{
    struct mem_size_stats *mss = walk->private;
    struct vm_area_struct *vma = walk->vma;
    bool locked = !!(vma->vm_flags & VM_LOCKED);
    struct page *page = NULL;

    if (pte_present(*pte)) {
        page = orig__vm_normal_page(vma, addr, *pte, false);
    } else if (is_swap_pte(*pte)) {
        swp_entry_t swpent = pte_to_swp_entry(*pte);

        if (!non_swap_entry(swpent)) {
            int mapcount;

            mss->swap += PAGE_SIZE;
            mapcount = orig_swp_swapcount(swpent);
            if (mapcount >= 2) {
                u64 pss_delta = (u64)PAGE_SIZE << PSS_SHIFT;

                do_div(pss_delta, mapcount);
                mss->swap_pss += pss_delta;
            } else {
                mss->swap_pss += (u64)PAGE_SIZE << PSS_SHIFT;
            }
        } else if (is_migration_entry(swpent))
            page = migration_entry_to_page(swpent);
        else if (is_device_private_entry(swpent))
            page = device_private_entry_to_page(swpent);
    } else if (unlikely(IS_ENABLED(CONFIG_SHMEM) && mss->check_shmem_swap
                            && pte_none(*pte))) {
        page = find_get_entry(vma->vm_file->f_mapping,
                        linear_page_index(vma, addr));
        if (!page)
            return;

        if (radix_tree_exceptional_entry(page))
            mss->swap += PAGE_SIZE;
        else
            put_page(page);

        return;
    }

    if (!page)
        return;

    smaps_account(mss, page, false, pte_young(*pte), pte_dirty(*pte), locked);
}

static void smaps_pmd_entry(pmd_t *pmd, unsigned long addr,
        struct mm_walk *walk)
{
    struct mem_size_stats *mss = walk->private;
    struct vm_area_struct *vma = walk->vma;
    bool locked = !!(vma->vm_flags & VM_LOCKED);
    struct page *page;

    /* FOLL_DUMP will return -EFAULT on huge zero page */
    page = orig_follow_trans_huge_pmd(vma, addr, pmd, FOLL_DUMP);
    if (IS_ERR_OR_NULL(page))
        return;
    if (PageAnon(page))
        mss->anonymous_thp += HPAGE_PMD_SIZE;
    else if (PageSwapBacked(page))
        mss->shmem_thp += HPAGE_PMD_SIZE;
    else if (is_zone_device_page(page))
        /* pass */;
    else
        VM_BUG_ON_PAGE(1, page);
    smaps_account(mss, page, true, pmd_young(*pmd), pmd_dirty(*pmd), locked);
}

static int smaps_pte_range(pmd_t *pmd, unsigned long addr, unsigned long end,
			 struct mm_walk *walk)
{
	struct vm_area_struct *vma = walk->vma;
	pte_t *pte;
	spinlock_t *ptl;

	if (is_swap_pmd(*pmd) || pmd_trans_huge(*pmd) || pmd_devmap(*pmd))
       ptl = orig___pmd_trans_huge_lock(pmd, vma);
    else
       ptl = NULL;

	if (ptl) {
		if (pmd_present(*pmd))
			smaps_pmd_entry(pmd, addr, walk);
		spin_unlock(ptl);
		goto out;
	}

	if (pmd_trans_unstable(pmd))
		goto out;

	/*
	* The mmap_sem held all the way back in m_start() is what
	* keeps khugepaged out of here and from collapsing things
	* in here.
	*/
	pte = pte_offset_map_lock(vma->vm_mm, pmd, addr, &ptl);
	for (; addr != end; pte++, addr += PAGE_SIZE)
		smaps_pte_entry(pte, addr, walk);
	pte_unmap_unlock(pte - 1, ptl);
out:
	cond_resched();
	return 0;
}

static void dump_mss_info(struct mem_size_stats *mss)
{
	int i;

	printk("resident:%ld \n",(unsigned long)mss->resident/PAGE_SIZE*4);
	printk("shared_clean:%ld \n",(unsigned long)mss->shared_clean/PAGE_SIZE*4);
	printk("shared_dirty:%ld \n",(unsigned long)mss->shared_dirty/PAGE_SIZE*4);
	printk("private_clean:%ld \n",(unsigned long)mss->private_clean/PAGE_SIZE*4);
	printk("private_dirty:%ld \n",(unsigned long)mss->private_dirty/PAGE_SIZE*4);
	printk("referenced:%ld \n",(unsigned long)mss->referenced/PAGE_SIZE*4);
	printk("anonymous:%ld \n",(unsigned long)mss->anonymous/PAGE_SIZE*4);
	printk("lazyfree:%ld \n",(unsigned long)mss->lazyfree/PAGE_SIZE*4);
	printk("anonymous_thp:%ld \n",(unsigned long)mss->anonymous_thp/PAGE_SIZE*4);
	printk("shmem_thp:%ld \n",(unsigned long)mss->shmem_thp/PAGE_SIZE*4);
	printk("swap:%ld \n",(unsigned long)mss->swap/PAGE_SIZE*4);
	printk("shared_hugetlb:%ld \n",(unsigned long)mss->shared_hugetlb/PAGE_SIZE*4);
	printk("private_hugetlb:%ld \n",(unsigned long)mss->private_hugetlb/PAGE_SIZE*4);
	printk("pss:%ld \n",(unsigned long)mss->pss/PAGE_SIZE*4);
	printk("pss_locked:%ld \n",(unsigned long)mss->pss_locked/PAGE_SIZE*4);
	printk("swap_pss:%ld \n",(unsigned long)mss->swap_pss/PAGE_SIZE*4);

	for (i = 0; i < 11; ++i) {
		printk("order %d %lx \n",(int)i, 	mss->page_order[i]);
	}
	printk("pages:%lx\n",	mss->page_index);
}

// it will scan all vma list of task
static void vma_scan(struct ioctl_data *data)
{
	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	int index = 0, i ;
	struct mem_size_stats mss;

	memset(&mss, 0, sizeof(mss));
	mss.page_buffer =  data->kmem_data.mss.page_buffer;

	printk("zz %s page_buffer:%lx  %lx\n",__func__, data->kmem_data.mss.page_buffer, mss.page_buffer);

	if (data->pid != -1)
			task = get_taskstruct_by_pid(data->pid);
	else
			task = current;

	if (task == NULL) {	
		printk("error vma scan not find task struct\n");
		return;
	}

	for (i = 0; i < NR_MM_COUNTERS; i++) {
			printk("name %s type:%d rss:%d \n", task->comm, i, task->rss_stat.count[i]);
 	}

	mm = task->mm;
	printk("vma scan task name:%s\n", task->comm);
	down_read(&mm->mmap_sem);

	for (vma = mm->mmap; vma; vma = vma->vm_next) {
#if 0
		data->log_buf += sprintf(data->log_buf, "%d: %lx~%lx\n", index++, vma->vm_start, vma->vm_end);
		vma_scan_map_page_list(task, vma);
#else
		struct mm_walk smaps_walk = {
			.pmd_entry = smaps_pte_range,
			.mm = vma->vm_mm,
		};
		smaps_walk.private = &mss;

		orig_walk_page_vma(vma, &smaps_walk);
#endif
	}

	up_read(&mm->mmap_sem);
	dump_mss_info(&mss);

	copy_to_user(&data->kmem_data.mss, &mss, sizeof(struct mem_size_stats));

}

int kmem_unit_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data)
{
	int ret = -1;

	switch (data->cmdcode) {
		case  IOCTL_USEKMEM_SHOW:
			DEBUG("mem_readlock_test_start\n")
			kmem_dump_state();
			break;

		case  IOCTL_USEKMEM_VMA_SCAN:
			DEBUG("mem vma scan\n")
			vma_scan(data);
			break;

		case  IOCTL_USERCU_READTEST_END:
			DEBUG("mem_readlock_test_stop\n")
			//mem_readlock_test_stop();
			break;
		default:
			goto OUT;
	}
OUT:
	return ret;
}

int kmem_unit_init(void)
{
	void *buf = kmalloc(PAGE_SIZE*3, GFP_KERNEL);
	printk("page:%d order:%d a2\n", virt_to_page(buf), compound_order(virt_to_page(buf)));
	kfree(buf);
	LOOKUP_SYMS(vm_zone_stat);
	LOOKUP_SYMS(vm_numa_stat);
	LOOKUP_SYMS(vm_node_stat);
	LOOKUP_SYMS(_vm_normal_page);
	LOOKUP_SYMS(arch_tlb_gather_mmu);
	LOOKUP_SYMS(walk_page_vma);
	LOOKUP_SYMS(swp_swapcount);
	LOOKUP_SYMS(__pmd_trans_huge_lock);
	LOOKUP_SYMS(follow_trans_huge_pmd);
	return 0;
}

int kmem_unit_exit(void)
{
	return 0;
}

