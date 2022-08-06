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
#include <linux/version.h>
#include <linux/page_idle.h>

#include "../template_iocmd.h"
#include "../misc_ioctl.h"
#include "../debug_ctrl.h"
#include "mekernel.h"

atomic_long_t *orig_vm_zone_stat;
atomic_long_t *orig_vm_numa_stat;
atomic_long_t *orig_vm_node_stat;

struct compact_control {
	struct list_head freepages;	/* List of free pages to migrate to */
	struct list_head migratepages;	/* List of pages being migrated */
	struct zone *zone;
	unsigned long nr_freepages;	/* Number of isolated free pages */
	unsigned long nr_migratepages;	/* Number of pages to migrate */
	unsigned long total_migrate_scanned;
	unsigned long total_free_scanned;
	unsigned long free_pfn;		/* isolate_freepages search base */
	unsigned long migrate_pfn;	/* isolate_migratepages search base */
	unsigned long last_migrated_pfn;/* Not yet flushed page being freed */
	const gfp_t gfp_mask;		/* gfp mask of a direct compactor */
	int order;			/* order a direct compactor needs */
	int migratetype;		/* migratetype of direct compactor */
	const unsigned int alloc_flags;	/* alloc flags of a direct compactor */
	const int classzone_idx;	/* zone index of a direct compactor */
	enum migrate_mode mode;		/* Async or sync migration mode */
	bool ignore_skip_hint;		/* Scan blocks even if marked skip */
	bool no_set_skip_hint;		/* Don't mark blocks for skipping */
	bool ignore_block_suitable;	/* Scan blocks considered unsuitable */
	bool direct_compaction;		/* False from kcompactd or /proc/... */
	bool whole_zone;		/* Whole zone should/has been scanned */
	bool contended;			/* Signal lock or sched contention */
	bool finishing_block;		/* Finishing current pageblock */
};

unsigned long (*orig_isolate_migratepages_block)(struct compact_control *cc, unsigned long low_pfn,
		unsigned long end_pfn, isolate_mode_t isolate_mode);


struct pglist_data *next_online_pgdat(struct pglist_data *pgdat)
{
	int nid = next_online_node(pgdat->node_id);
	if (nid == MAX_NUMNODES)
		return NULL;
	return NODE_DATA(nid);
}

struct zone *next_zone(struct zone *zone)
{
	pg_data_t *pgdat = zone->zone_pgdat;
	if (zone < pgdat->node_zones + MAX_NR_ZONES - 1)
		zone++;
	else {
		pgdat = next_online_pgdat(pgdat);
		if (pgdat)
			zone = pgdat->node_zones;
		else
			zone = NULL;
	}
	return zone;
}

struct pglist_data *first_online_pgdat(void)
{
	return NODE_DATA(first_online_node);
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
#if LINUX_VERSION_CODE <  KERNEL_VERSION(5,0,0)
	printk("NR_VM_NUMA_STAT_ITEMS: %ld\n", atomic_long_read(&orig_vm_numa_stat[NR_VM_NUMA_STAT_ITEMS]));
#else
	printk("NR_VM_NUMA_STAT_ITEMS: %ld\n", atomic_long_read(&orig_vm_numa_stat[NR_VM_NODE_STAT_ITEMS]));
#endif
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
#if LINUX_VERSION_CODE <  KERNEL_VERSION(5,0,0)
	printk("NR_SLAB_RECLAIMABLE: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_SLAB_RECLAIMABLE]));
	printk("NR_SLAB_UNRECLAIMABLE: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_SLAB_UNRECLAIMABLE]));
#endif
	printk("NR_ISOLATED_ANON: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_ISOLATED_ANON]));
	printk("NR_ISOLATED_FILE: %ld\n", atomic_long_read(&orig_vm_node_stat[NR_ISOLATED_FILE]));
#if LINUX_VERSION_CODE <  KERNEL_VERSION(4,19,0)
	printk("WORKINGSET_REFAULT: %ld\n", atomic_long_read(&orig_vm_node_stat[WORKINGSET_REFAULT]));
	printk("WORKINGSET_ACTIVATE: %ld\n", atomic_long_read(&orig_vm_node_stat[WORKINGSET_ACTIVATE]));
#endif
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

void kmem_dump_state(void)
{
	kmem_dump_zone_item(orig_vm_zone_stat);
	kmem_dump_node_item(orig_vm_node_stat);
#ifdef CONFIG_NUMA
	kmem_dump_numa_item();
#endif
}

void compatc_zone_order(struct zone *zone, int order, gfp_t gfp_mask, unsigned int alloc_flags, int classzone_idx)
{
	u64 start;
	u64 old_mirgrate_pfn = 0;
	struct page *page;
	struct compact_control cc = {
		.nr_freepages = 0,
		.nr_migratepages = 0,
		.total_migrate_scanned = 0,
		.total_free_scanned = 0,
		.order = order,
		.gfp_mask = gfp_mask,
		.zone = zone,
		.mode = MIGRATE_SYNC,
		.alloc_flags = alloc_flags,
		.classzone_idx = classzone_idx,
		.direct_compaction = true,
		.whole_zone = 1,
		.ignore_skip_hint = 0,
		.ignore_block_suitable = 0,
	};
	INIT_LIST_HEAD(&cc.freepages);
	INIT_LIST_HEAD(&cc.migratepages);
	//orig_isolate_migratepages_block(&cc, 0x400000, 0x500000, MIGRATE_SYNC);
	for (start = 0x400000; start < 0x500000; start += 0x200) {
		page = pfn_to_page(start);
		if (!PageLRU(page)) {
			//printk("zz %s %d not in lru\n", __func__, __LINE__);
		}	else
			orig_isolate_migratepages_block(&cc, start, start + 0x200, ISOLATE_UNEVICTABLE );
		if ( old_mirgrate_pfn != cc.nr_migratepages) {

			old_mirgrate_pfn = cc.nr_migratepages;
			printk("start:%llx cc->nr_freepages:%lx cc->nr_migratepages:%lx \n", start, (unsigned long)cc.nr_freepages, (unsigned long)cc.nr_migratepages);
		}
	}
}

void zone_dump_info(struct ioctl_data *data)
{
	struct zone_data *zdata = &data->kmem_data.zonedata;
	struct zone *zone;
	for_each_populated_zone(zone) {
		//zdata->pageblock_flags = zone->pageblock_flags;
		zdata->zone_start_pfn = zone->zone_start_pfn;
#if LINUX_VERSION_CODE <  KERNEL_VERSION(5,0,0)
		zdata->managed_pages = zone->managed_pages;
#endif
		zdata->spanned_pages = zone->spanned_pages;
		zdata->present_pages = zone->present_pages;
		zdata->nr_isolate_pageblock = zone->nr_isolate_pageblock;
		zdata->compact_cached_free_pfn = zone->compact_cached_free_pfn;
		zdata->compact_cached_migrate_pfn[0] = zone->compact_cached_migrate_pfn[0];
		zdata->compact_cached_migrate_pfn[1] = zone->compact_cached_migrate_pfn[1];
		compatc_zone_order(zone, 9, 0, 0, 0);
		printk("zz %s %d \n", __func__, __LINE__);
	}
}


