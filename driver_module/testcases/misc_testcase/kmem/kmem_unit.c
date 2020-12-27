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
#include <asm/tlb.h>

#include "../template_iocmd.h"
#include "../misc_ioctl.h"
#include "../debug_ctrl.h"
#include "mekernel.h"
#include "kmemlocal.h"

#define PSS_SHIFT 12

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
}

// it will scan all vma list of task
static int vma_scan(struct ioctl_data *data)
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

		printk("zz %s vm_start:%lx vm_end:%lx \n",__func__, (unsigned long)vma->vm_start, (unsigned long)vma->vm_end);
		//orig_walk_page_vma(vma, &smaps_walk);
		vma_pte_dump(vma, vma->vm_start, (vma->vm_end - vma->vm_start)>>PAGE_SHIFT);
#endif
	}

	up_read(&mm->mmap_sem);
	dump_mss_info(&mss);

	return copy_to_user(&data->kmem_data.mss, &mss, sizeof(struct mem_size_stats));
}

static void dump_kernel_page_attr(struct ioctl_data *data, u64 start_pfn, u64 size)
{
	struct page *page;
	int i;

	for (i = 0; i < size; ++i) {
		page = pfn_to_page(start_pfn + i);
		printk("page:%llx-%llx page_flags:%llx\n", (u64)start_pfn + i, (u64)page, (u64)page->flags);
	}
}

//#define __START_KERNEL_map_test  (0xffffffff8000000UL)
#define __START_KERNEL_map_test  _AC(0xffffffff80000000, UL)

static inline unsigned long __mephys_addr_nodebug(unsigned long x)
{
	 unsigned long y = x - __START_KERNEL_map;
	 printk("zz %s %d %llx\n", __func__, __LINE__, x - __START_KERNEL_map_test);
	 printk("zz %s x:%llx y:%llx %llx\n",__func__, (unsigned long)x, (unsigned long)y, __START_KERNEL_map);
	 printk("zz %s __START_KERNEL_map:%lx phys_base:%lx PAGE_OFFSET:%lx page_offset_base:%lx vmemmap_base::%llx \n",__func__, (unsigned long)__START_KERNEL_map, (unsigned long)phys_base, (unsigned long)PAGE_OFFSET, (unsigned long)page_offset_base, vmemmap_base);
	 printk("zz %s a:%llx b:%llx c:%llx  %llx %llx\n",__func__, x, x - __START_KERNEL_map, y + phys_base, __START_KERNEL_map - PAGE_OFFSET, y + __START_KERNEL_map - PAGE_OFFSET);
	 x = y + ((x > y) ? phys_base : (__START_KERNEL_map - PAGE_OFFSET));

	 return x;
}

static void page_test(void)
{
	struct zone *zone;
	void *buf;
	u64 pfn;
	struct page *page;
	struct per_cpu_pages *pcp;
	struct pglist_data *__pgdat;

	buf = kmalloc(PAGE_SIZE * 4 , GFP_KERNEL);
	page = virt_to_page(buf);
	//page = (struct page *) 0xffffea5d145eb300;
	zone = page_zone(page);
	pcp = &this_cpu_ptr(zone->pageset)->pcp;

#if 1
	//page = pfn_to_page(0x2ddd2e);
	pfn = page_to_pfn(page);

	//__pgdat = NODE_DATA(page_to_nid(page));
	printk("zz %s page:%lx phys:%lx %llx %llx pfn:%lx\n",__func__, (unsigned long)page, virt_to_phys(page)
			,  __mephys_addr_nodebug(page), phys_base, pfn);
	dump_page_info(page);

#if 0
	printk("%llx: %llx %llx buddy:%d Comp:%d LRU:%d map:%llx order:%llx count:%d head:%lx free:%lx pcp:%lx\n", page_to_pfn(page), page->page_type, page->flags, PageBuddy(page),
			 PageCompound(page), PageLRU(page), page_mapping(page), page_private(page), page_count(page), page_to_pfn(compound_head(page)), page->index
			 ,pcp->count);
#endif
	//trace_printk("+\n");
	//atomic_set(&page->_refcount, 1);
	//put_page(page);
	//kfree(buf);

#if 0
	printk("%llx: %llx %llx buddy:%d Comp:%d LRU:%d map:%llx order:%llx count:%d head:%lx free:%lx pcp:%lx\n", page_to_pfn(page), page->page_type, page->flags, PageBuddy(page),
			 PageCompound(page), PageLRU(page), page_mapping(page), page_private(page), page_count(page), page_to_pfn(compound_head(page)), page->index
			 ,pcp->count);
#endif
#else
	kfree(buf);
#endif
}

int kmem_unit_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data)
{
	int ret = -1;
	pte_t *pte;

	printk("zz %s %d cmdcode:%lx\n", __func__, __LINE__, data->cmdcode);

	switch (data->cmdcode) {
		case  IOCTL_USEKMEM_SHOW:
			DEBUG("mem_readlock_test_start\n")
			kmem_dump_state();
			break;

		case  IOCTL_USEKMEM_VMA_SCAN:
			printk("zz %s %d \n", __func__, __LINE__);
			DEBUG("mem vma scan\n")
			vma_scan(data);
			break;

		case  IOCTL_USEKMEM_GET_PTE:
			DEBUG("mem pte get\n")
			pte = get_pte(data->kmem_data.addr, current->mm);
			data->kmem_data.val = *((u64*) pte);
			dump_pte_info(pte);
			break;

		case  IOCTL_USERCU_READTEST_END:
			DEBUG("mem_readlock_test_stop\n")
			//mem_readlock_test_stop();
			break;

		case IOCTL_USEKMEM_PAGE_ATTR:
			DEBUG("kmem page attr\n")
			dump_kernel_page_attr(data, data->kmem_data.pageattr_data.start_pfn,
					data->kmem_data.pageattr_data.size);
			break;

		default:
			goto OUT;
	}
OUT:
	return ret;
}

int kmem_unit_init(void)
{
	LOOKUP_SYMS(vm_zone_stat);
	LOOKUP_SYMS(vm_numa_stat);
	LOOKUP_SYMS(vm_node_stat);
	LOOKUP_SYMS(_vm_normal_page);
	LOOKUP_SYMS(arch_tlb_gather_mmu);
	LOOKUP_SYMS(walk_page_vma);
	LOOKUP_SYMS(swp_swapcount);
	LOOKUP_SYMS(__pmd_trans_huge_lock);
	LOOKUP_SYMS(follow_trans_huge_pmd);
	LOOKUP_SYMS(isolate_migratepages_block);
	{
		page_test();
		//struct ioctl_data data;	
		//zone_dump_info(&data);
	}
	return 0;
}

int kmem_unit_exit(void)
{
	return 0;
}

