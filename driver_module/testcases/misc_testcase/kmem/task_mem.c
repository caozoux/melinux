#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/version.h>
#include <linux/page_idle.h>
#include <linux/swap.h>
#include <linux/swapops.h>
#include <linux/slab.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE >  KERNEL_VERSION(5,0,0)
#include <linux/pagewalk.h>
#endif

#include "../template_iocmd.h"
#include "../misc_ioctl.h"
#include "../debug_ctrl.h"
#include "mekernel.h"

#define PSS_SHIFT 12

extern spinlock_t *(*orig___pmd_trans_huge_lock)(pmd_t *pmd, struct vm_area_struct *vma);
extern struct page *(*orig_follow_trans_huge_pmd)(struct vm_area_struct *vma,
                   unsigned long addr,
                   pmd_t *pmd,
                   unsigned int flags);
extern int (*orig_swp_swapcount)(swp_entry_t entry);
extern struct page *(*orig__vm_normal_page)(struct vm_area_struct *vma, unsigned long addr, pte_t pte, bool with_public_device);

#if LINUX_VERSION_CODE <  KERNEL_VERSION(5,0,0)
static void smaps_account(struct mem_size_stats *mss, struct page *page,
        bool compound, bool young, bool dirty, bool locked)
{
    int i, nr = compound ? 1 << compound_order(page) : 1;
    unsigned long size = nr * PAGE_SIZE;
	unsigned long phys;
	int ret;

	mss->page_order[compound_order(page)]++;

	phys = page_to_phys(page);
	ret = copy_to_user(mss->page_buffer + mss->page_index, &phys, sizeof(unsigned long));
	if (!ret)
		return;

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
#if LINUX_VERSION_CODE >  KERNEL_VERSION(4,19,0)
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
#endif
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
#endif

static int smaps_pte_range(pmd_t *pmd, unsigned long addr, unsigned long end,
			 struct mm_walk *walk)
{
#if LINUX_VERSION_CODE <  KERNEL_VERSION(5,0,0)
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
#endif
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
}

// it will scan all vma list of task
int vma_scan(struct ioctl_data *data)
{
	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	int i;
	struct mem_size_stats mss;

	memset(&mss, 0, sizeof(mss));
	mss.page_buffer =  data->kmem_data.mss.page_buffer;

	printk("zz %s data->pid:%08x \n",__func__, (int)data->pid);
	if (data->pid != -1)
			task = get_taskstruct_by_pid(data->pid);
	else
			task = current;

	if (task == NULL) {	
		printk("error vma scan not find task struct\n");
		return 0;
	}

	for (i = 0; i < NR_MM_COUNTERS; i++) {
		printk("name %s type:%d rss:%d \n", task->comm, i, task->rss_stat.count[i]);
 	}

	mm = task->mm;

#if LINUX_VERSION_CODE <  KERNEL_VERSION(5,0,0)
	down_read(&mm->mmap_sem);
#else
	mmap_read_lock(mm);
#endif

	for (vma = mm->mmap; vma; vma = vma->vm_next) {
#if 0
		data->log_buf += sprintf(data->log_buf, "%d: %lx~%lx\n", index++, vma->vm_start, vma->vm_end);
		vma_scan_map_page_list(task, vma);
#else
#if LINUX_VERSION_CODE <  KERNEL_VERSION(5,0,0)
		struct mm_walk smaps_walk = {
			.pmd_entry = smaps_pte_range,
			.mm = vma->vm_mm,
		};
		smaps_walk.private = &mss;
		//orig_walk_page_vma(vma, &smaps_walk);
#else
		struct mm_walk_ops smaps_walk = {
			.pmd_entry = smaps_pte_range,
		};
		//orig_walk_page_vma(vma, &smaps_walk, NULL);
#endif

		vma_pte_dump(vma, vma->vm_start, (vma->vm_end - vma->vm_start)>>PAGE_SHIFT);
#endif
	}

#if LINUX_VERSION_CODE <  KERNEL_VERSION(5,0,0)
	up_read(&mm->mmap_sem);
#else
	mmap_read_unlock(mm);
#endif

	dump_mss_info(&mss);

	return copy_to_user(&data->kmem_data.mss, &mss, sizeof(struct mem_size_stats));
}

