#ifndef __KMEM_LOCAL_H__
#define __KMEM_LOCAL_H__

extern atomic_long_t *orig_vm_zone_stat;
extern atomic_long_t *orig_vm_numa_stat;
extern atomic_long_t *orig_vm_node_stat;
void kmem_dump_state(void);
extern unsigned long (*orig_isolate_migratepages_block)(struct compact_control *cc, unsigned long low_pfn,
		unsigned long end_pfn, isolate_mode_t isolate_mode);

void compatc_zone_order(struct zone *zone, int order, gfp_t gfp_mask, unsigned int alloc_flags, int classzone_idx);
void zone_dump_info(struct ioctl_data *data);
#endif

