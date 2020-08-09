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

#include "../template_iocmd.h"
#include "../misc_ioctl.h"
#include "../debug_ctrl.h"

atomic_long_t *orig_vm_zone_stat;
atomic_long_t *orig_vm_numa_stat;
atomic_long_t *orig_vm_node_stat;

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
	printk("WORKINGSET_RESTORE: %ld\n", atomic_long_read(&orig_vm_node_stat[WORKINGSET_RESTORE]));
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
	printk("NR_UNSTABLE_NFS: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_UNSTABLE_NFS]));
	printk("NR_VMSCAN_WRITE: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_VMSCAN_WRITE]));
	printk("NR_VMSCAN_IMMEDIATE: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_VMSCAN_IMMEDIATE]));
	printk("NR_DIRTIED: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_DIRTIED]));
	printk("NR_WRITTEN: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_WRITTEN]));
	printk("NR_INDIRECTLY_RECLAIMABLE_BYTES: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_INDIRECTLY_RECLAIMABLE_BYTES]));
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

int kmem_unit_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data)
{
	int ret = -1;
	switch (data->cmdcode) {
		case  IOCTL_USEMEM_SHOW:
			DEBUG("mem_readlock_test_start\n")
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
	kmem_dump_state();
	return 0;
}

int kmem_unit_exit(void)
{
	return 0;
}

