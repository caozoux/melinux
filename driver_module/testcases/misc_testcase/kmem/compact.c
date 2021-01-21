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
#include <linux/compaction.h>
#include <asm/tlb.h>

#include "../template_iocmd.h"
#include "../misc_ioctl.h"
#include "../debug_ctrl.h"
#include "mekernel.h"
#include "kmemlocal.h"

struct alloc_context {                                                                                                                                                                                  
    struct zonelist *zonelist;
    nodemask_t *nodemask;
    struct zoneref *preferred_zoneref;
    int migratetype;
    enum zone_type high_zoneidx;
    bool spread_dirty_pages;
};

struct page * (*orig___alloc_pages_direct_compact)(gfp_t gfp_mask, unsigned int order,
  unsigned int alloc_flags, const struct alloc_context *ac,
  enum compact_priority prio, int *compact_result);

void kmem_compact(gfp_t gfp_mask, unsigned int order)
{
       struct page *page = NULL;
       unsigned int alloc_flags;
       //enum compact_result compact_result;
       int compact_result;
       int node = 0;
       struct zonelist *zonelist =  node_zonelist(node, gfp_mask);

       struct alloc_context ac = {
               .high_zoneidx = gfp_zone(gfp_mask),
               .zonelist = node_zonelist(node, gfp_mask),
               .nodemask = NULL,
               .migratetype = gfpflags_to_migratetype(gfp_mask),
       };

       alloc_flags = GFP_KERNEL;

#if 1
       page = orig___alloc_pages_direct_compact(gfp_mask, order,
                                       alloc_flags, &ac,
                                       INIT_COMPACT_PRIORITY,
                                       &compact_result);
#endif
}
//EXPORT_SYMBOL(test_compact);


