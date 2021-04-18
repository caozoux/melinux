#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/slab.h>
#include <linux/slub_def.h>
#include <linux/delay.h>
#include <linux/swap.h>
#include <linux/swapops.h>
#include <linux/page_idle.h>
#include <linux/version.h>
//#include <asm/tlb.h>

#include "../template_iocmd.h"
#include "../misc_ioctl.h"
#include "../debug_ctrl.h"
#include "mekernel.h"
#include "kmemlocal.h"

struct mem_cgroup *orig_mem_cgroup_iter(struct mem_cgroup *root, struct mem_cgroup *prev,
								struct mem_cgroup_reclaim_cookie *reclaim);

struct mem_cgroup *orig_root_mem_cgroup;

#define for_each_mem_cgroup_tree(iter, root)        \
	for (iter = mem_cgroup_iter(root, NULL, NULL);  \
			iter != NULL;              \
			iter = mem_cgroup_iter(root, iter, NULL))


#define for_each_mem_cgroup(iter)           \
	for (iter = mem_cgroup_iter(NULL, NULL, NULL);  \
			iter != NULL;              \
			iter = mem_cgroup_iter(NULL, iter, NULL))


