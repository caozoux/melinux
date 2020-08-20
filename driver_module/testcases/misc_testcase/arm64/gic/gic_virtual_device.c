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
static struct irq_domain_ops *orig_its_domain_ops;
static struct irq_domain *its_domain;
static msi_alloc_info_t *gic_virt_msi_inif;
static struct msi_desc gic_virt_msi_desc;

static int (*orig_its_msi_prepare)(struct irq_domain *domain, struct device *dev,
	int nvec, msi_alloc_info_t *info);
int (*orig___irq_domain_alloc_irqs)(struct irq_domain *domain, int irq_base,
				unsigned int nr_irqs, int node, void *arg,
				bool realloc, const struct cpumask *affinity);
int (*orig_irq_domain_activate_irq)(struct irq_data *irq_data, bool reserve);
void (*orig_irq_domain_free_irqs)(unsigned int virq, unsigned int nr_irqs);

static irqreturn_t gic_virtual_device_irq(int irq, void *dev_id)
{
	printk("%s\n", __func__);
	return IRQ_HANDLED;
}

static void irq_domain_list(void)
{
	struct irq_domain *h;
	struct device *dev =  virtual_test_get_device();
	struct irq_data *irq_data;
	int nvec = 8, ret;
	 struct irq_chip *chip;
	int irq;
	bool val = 1;
	
	list_for_each_entry(h, orig_irq_domain_list, link) {
		//printk("irq_domain %s\n", h->name);
		if (h->ops == orig_its_domain_ops) {
			its_domain = h;
			break;
		}
	}

	if (!its_domain)
		return -ENODEV;
	
	gic_virt_msi_inif = kzalloc(sizeof(msi_alloc_info_t), GFP_KERNEL);
	gic_virt_msi_inif->scratchpad[0].ul = 18;
	gic_virt_msi_desc.dev = virtual_test_get_device();
	gic_virt_msi_inif->desc = &gic_virt_msi_desc;

	orig_its_msi_prepare(its_domain, dev, nvec, gic_virt_msi_inif);

	irq = orig___irq_domain_alloc_irqs(its_domain, -1, 1,	
		NUMA_NO_NODE, gic_virt_msi_inif,false, NULL);
	irq_data = irq_domain_get_irq_data(its_domain, irq);
	ret = orig_irq_domain_activate_irq(irq_data, 0);
	ret = request_irq(irq, gic_virtual_device_irq, 0, "cpm_i2c", NULL);

	if (ret) 
		printk("register irq %d failed\n", irq);
	
	irq_data->chip->irq_set_irqchip_state(irq_data, IRQCHIP_STATE_PENDING, &val);
	udelay(100);
	irq_data->chip->irq_set_irqchip_state(irq_data, IRQCHIP_STATE_PENDING, &val);
}

int gic_virtual_device_init(void)
{
	LOOKUP_SYMS(irq_domain_list);
	LOOKUP_SYMS(its_domain_ops);
	LOOKUP_SYMS(its_msi_prepare);
	LOOKUP_SYMS(__irq_domain_alloc_irqs);
	LOOKUP_SYMS(irq_domain_activate_irq);
	LOOKUP_SYMS(irq_domain_free_irqs);
	irq_domain_list();
}

int gic_virtual_device_exit(void)
{
	if (gic_virt_msi_inif)
		kfree(gic_virt_msi_inif);
}

