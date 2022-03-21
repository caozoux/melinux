#ifndef __KMEM_LOCAL_H__
#define __KMEM_LOCAL_H__
#include "vma.h"

extern atomic_long_t *orig_vm_zone_stat;
extern atomic_long_t *orig_vm_numa_stat;
extern atomic_long_t *orig_vm_node_stat;
extern struct resource *orig_iomem_resource;
extern void (*orig_page_idle_clear_pte_refs)(struct page *page);

void kmem_dump_state(void);
void compatc_zone_order(struct zone *zone, int order, gfp_t gfp_mask, unsigned int alloc_flags, int classzone_idx);
void zone_dump_info(struct ioctl_data *data);
void page_info_show(struct page *page);
int kmem_kmemcache_create(char *name, int size);
int kmem_kmemcache_create_objs(char *name, int size, int is_free);
void kmem_kmemcache_remove(char *name);
void  start_node_scan_thread(void);
void resource_scan(void);
int enumerate_node_memcg(void);
int vma_scan(struct ioctl_data *data);
int get_page_rmap_addr(struct page *old);


//hugetlb
void enum_hugetlb(void);
#endif

