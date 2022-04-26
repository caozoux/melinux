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
//#include <asm/tlb.h>

#include "../template_iocmd.h"
#include "../misc_ioctl.h"
#include "../debug_ctrl.h"
#include "mekernel.h"

struct resource *orig_iomem_resource;
struct resource *next_resource(struct resource *p)
{
	if (p->child) 
		return p->child;
	while (!p->sibling && p->parent)
		p = p->parent;
	return p->sibling;

}
void resource_scan(void)
{
	struct resource *p;
	for (p = orig_iomem_resource->child; p; p = next_resource(p)) {
		printk("zz %s name:%s start:%lx end:%lx \n",__func__, p->name, (unsigned long)p->start, (unsigned long)p->end);
	}
}
