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
#include <linux/pci.h>
#include <linux/msi.h>
#include <linux/irq.h>
#include <linux/list.h>
#include <linux/irqdomain.h>

#ifdef CONFIG_ARM64
#include <linux/irqchip/arm-gic-common.h>
#include <linux/irqchip/arm-gic-v3.h>
#endif
#include "../template_iocmd.h"
#include "../misc_ioctl.h"
#include "../debug_ctrl.h"
#include "gic_local_data.h"
#include "mekernel.h"


static struct list_head *orig_irq_domain_list;
static struct irq_domain_ops *orig_its_domain_ops
static struct irq_domain *its_domain_ops;
static msi_alloc_info_t *gic_virt_msi_inif;

static int orig_its_msi_prepare(struct irq_domain *domain, struct device *dev,
	int nvec, msi_alloc_info_t *info);

static void irq_domain_list(void)
{
	struct irq_domain *h;
	struct pci_dev *dev;
	
	list_for_each_entry(h, orig_irq_domain_list, link) {
		//printk("irq_domain %s\n", h->name);		
		if (h->ops == orig_its_domain_ops) {
			its_domain_ops = h;
			break;
		}
	}

	if (!its_domain_ops)
		return -ENODEV;
	
	gic_virt_msi_inif = kzalloc(sizeof(struct msi_alloc_info_t), GFP_KERNEL);
	pdev = pci_unit_register_pci_device();
	info->scratchpad[0].ul = 18;

	orig_its_msi_prepare(its_domain_ops, pci_dev->dev, gic_virt_msi_inif);
		
	

}

int gic_virtual_device_init(void)
{
	LOOKUP_SYMS(irq_domain_list);
	LOOKUP_SYMS(its_domain_ops);
	LOOKUP_SYMS(its_msi_prepare);
	irq_domain_list();
}

int gic_virtual_device_exit(void)
{
	if (gic_virt_msi_inif)
		kfree(gic_virt_msi_inif)
}

