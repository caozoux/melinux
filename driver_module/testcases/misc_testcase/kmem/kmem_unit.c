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
#include <asm/tlb.h>

#include "../template_iocmd.h"
#include "../misc_ioctl.h"
#include "../debug_ctrl.h"
#include "mekernel.h"

atomic_long_t *orig_vm_zone_stat;
atomic_long_t *orig_vm_numa_stat;
atomic_long_t *orig_vm_node_stat;
//int (*orig_zap_huge_pud)(struct mmu_gather *tlb, struct vm_area_struct *vma, pud_t *pud, unsigned long addr);
struct page *(*orig__vm_normal_page)(struct vm_area_struct *vma, unsigned long addr, pte_t pte, bool with_public_device);
void (*orig_arch_tlb_gather_mmu)(struct mmu_gather *tlb, struct mm_struct *mm, unsigned long start, unsigned long end);

         

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

void pgd_clear_bad(pgd_t *pgd)
{
    pgd_ERROR(*pgd);
    pgd_clear(pgd);
}

void p4d_clear_bad(p4d_t *p4d)
{
    p4d_ERROR(*p4d);
    p4d_clear(p4d);
}

void pud_clear_bad(pud_t *pud)
{
    pud_ERROR(*pud);
    pud_clear(pud);
}

void pmd_clear_bad(pmd_t *pmd)
{
    pmd_ERROR(*pmd);
    pmd_clear(pmd);
}

static int rss_page_cnt = 0;
static unsigned long zap_pte_range(struct mmu_gather *tlb,
                struct vm_area_struct *vma, pmd_t *pmd,
                unsigned long addr, unsigned long end,
                struct zap_details *details)
{
	struct mm_struct *mm = tlb->mm;
	spinlock_t *ptl;
	pte_t *start_pte;
	pte_t *pte;

	start_pte = pte_offset_map_lock(mm, pmd, addr, &ptl);
    pte = start_pte;
	do {
		pte_t ptent = *pte;

        if (pte_none(ptent))
            continue;

        if (pte_present(ptent)) {
            struct page *page;
			page = orig__vm_normal_page(vma, addr, ptent, true);
			if (unlikely(!page))
				continue;
			rss_page_cnt++;
		}
	} while (pte++, addr += PAGE_SIZE, addr != end);
	pte_unmap_unlock(start_pte, ptl);

	return addr;
}

static inline unsigned long zap_pmd_range(struct mmu_gather *tlb,
                struct vm_area_struct *vma, pud_t *pud,
                unsigned long addr, unsigned long end,
				struct zap_details *details)
{
    pmd_t *pmd;
    unsigned long next;

    pmd = pmd_offset(pud, addr);
    do {
        next = pmd_addr_end(addr, end);
        if (is_swap_pmd(*pmd) || pmd_trans_huge(*pmd) || pmd_devmap(*pmd)) {
            //if (next - addr != HPAGE_PMD_SIZE)
            //   __split_huge_pmd(vma, pmd, addr, false, NULL);
            //else if (zap_huge_pmd(tlb, vma, pmd, addr))
            //   goto next;
            /* fall through */
        }
        /*
         * Here there can be other concurrent MADV_DONTNEED or
         * trans huge page faults running, and if the pmd is
         * none or trans huge it can change under us. This is
         * because MADV_DONTNEED holds the mmap_sem in read
         * mode.
         */
        if (pmd_none_or_trans_huge_or_clear_bad(pmd))
            goto next;
        next = zap_pte_range(tlb, vma, pmd, addr, next, details);
next:
        cond_resched();
    } while (pmd++, addr = next, addr != end);

    return addr;
}

static inline unsigned long zap_pud_range(struct mmu_gather *tlb,
                struct vm_area_struct *vma, p4d_t *p4d,
                unsigned long addr, unsigned long end,
				struct zap_details *details)
{
    pud_t *pud;
    unsigned long next;

    pud = pud_offset(p4d, addr);
    do {
        next = pud_addr_end(addr, end);
        if (pud_trans_huge(*pud) || pud_devmap(*pud)) {
            if (next - addr != HPAGE_PUD_SIZE) {
                VM_BUG_ON_VMA(!rwsem_is_locked(&tlb->mm->mmap_sem), vma);
            //   split_huge_pud(vma, pud, addr);
			}
            //} else if (zap_huge_pud(tlb, vma, pud, addr))
            //   goto next;
            /* fall through d*/
        }
        if (pud_none_or_clear_bad(pud))
            continue;
        next = zap_pmd_range(tlb, vma, pud, addr, next, details);
next:
        cond_resched();
    } while (pud++, addr = next, addr != end);

    return addr;
}

static unsigned long zap_p4d_range(struct mmu_gather *tlb,
                struct vm_area_struct *vma, pgd_t *pgd,
                unsigned long addr, unsigned long end,
				struct zap_details *details)
{
    p4d_t *p4d;
    unsigned long next;

    p4d = p4d_offset(pgd, addr);
    do {
        next = p4d_addr_end(addr, end);
        if (p4d_none_or_clear_bad(p4d))
            continue;
        next = zap_pud_range(tlb, vma, p4d, addr, next, details);
    } while (p4d++, addr = next, addr != end);

    return addr;
}

static void vma_scan_map_page_list(struct task_struct *task, struct vm_area_struct *p)
{
	struct vm_area_struct *vma = p;
	unsigned long addr = vma->vm_start;
	unsigned long end = vma->vm_end;
	unsigned long next;
	pgd_t *pgd;
	struct mmu_gather tlb;

	orig_arch_tlb_gather_mmu(&tlb, task->mm, addr, end);

	pgd = pgd_offset(vma->vm_mm, addr);
	do {
		next = pgd_addr_end(addr, end);
	    if (pgd_none_or_clear_bad(pgd))
			continue;
		next = zap_p4d_range(&tlb, vma, pgd, addr, next, NULL);
	} while (pgd++, addr = next, addr != end);
	//tlb_finish_mmu(&tlb, start, end)
}

// it will scan all vma list of task
static void vma_scan(struct ioctl_data *data)
{
	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	int index = 0, i ;


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
	rss_page_cnt = 0;
	down_read(&mm->mmap_sem);

	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		data->log_buf += sprintf(data->log_buf, "%d: %lx~%lx\n", index++, vma->vm_start, vma->vm_end);
		vma_scan_map_page_list(task, vma);
	}

	printk("zz %s rss_page_cnt:%d \n",__func__, (int)rss_page_cnt);
	up_read(&mm->mmap_sem);

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
	LOOKUP_SYMS(vm_zone_stat);
	LOOKUP_SYMS(vm_numa_stat);
	LOOKUP_SYMS(vm_node_stat);
	LOOKUP_SYMS(_vm_normal_page);
	LOOKUP_SYMS(arch_tlb_gather_mmu);
	return 0;
}

int kmem_unit_exit(void)
{
	return 0;
}

