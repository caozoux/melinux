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
#include <linux/rmap.h>
#include <linux/mm_inline.h>
//#include <asm/tlb.h>

#include "../template_iocmd.h"
#include "../misc_ioctl.h"
#include "../debug_ctrl.h"
#include "mekernel.h"

struct anon_vma_chain *
(*orig_anon_vma_interval_tree_iter_first)(struct rb_root_cached *root,
		unsigned long first, unsigned long last);

struct anon_vma_chain *
(*orig_anon_vma_interval_tree_iter_next)(struct anon_vma_chain *node,
	  unsigned long first, unsigned long last);

