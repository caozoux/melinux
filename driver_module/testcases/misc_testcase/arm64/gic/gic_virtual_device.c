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

#define IRQ_TRIGGER 	"irq_trigger"
#define IRQ_MASK    	"irq_mask"
#define IRQ_UNMASK  	"irq_unmask"
#define IRQ_PENDING 	"irq_pending"

static int gic_vdev_nrirqs = 1;
module_param(gic_vdev_nrirqs, int, S_IRUGO);
MODULE_PARM_DESC(gic_vdev_nrirqs, "virtaul device request irqs");

static LIST_HEAD(virtual_gic_dev_list);

struct gic_vdev_irq_attr;

struct virtual_gic_dev_data {
	int irq;
	int nrirqs;
	struct list_head entry;
	struct device *virt_dev;
	msi_alloc_info_t msi_info;
	struct msi_desc msi_desc;
	struct gic_vdev_irq_attr **vdev_irq_attr;
	char **attr_name;
};

#define IRQ_ATRS_NUM 4

struct gic_vdev_irq_attr {
	int irq;
	struct device_attribute dev_irq_attr;
	struct device_attribute dev_irq_mask;
	struct device_attribute dev_irq_unmask;
	struct device_attribute dev_irq_pending;
	struct attribute *irq_attrs[IRQ_ATRS_NUM+1];
	struct attribute_group group;
	const char name[16];
};

static struct list_head *orig_irq_domain_list;
static struct irq_domain_ops *orig_its_domain_ops;
static struct irq_domain *its_domain;

static int (*orig_its_msi_prepare)(struct irq_domain *domain, struct device *dev,
	int nvec, msi_alloc_info_t *info);
int (*orig___irq_domain_alloc_irqs)(struct irq_domain *domain, int irq_base,
				unsigned int nr_irqs, int node, void *arg,
				bool realloc, const struct cpumask *affinity);
int (*orig_irq_domain_activate_irq)(struct irq_data *irq_data, bool reserve);
void (*orig_irq_domain_free_irqs)(unsigned int virq, unsigned int nr_irqs);
void (*orig_irq_domain_deactivate_irq)(struct irq_data *irq_data);
void (*orig_irq_domain_free_irqs)(unsigned int virq, unsigned int nr_irqs);

static ssize_t store_irq_trigger(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct gic_vdev_irq_attr *vdev_irq_attr =
		container_of(attr, struct gic_vdev_irq_attr, dev_irq_attr);
	struct irq_data *irq_data;
	bool val = true;

	irq_data = irq_domain_get_irq_data(its_domain,vdev_irq_attr->irq);
	irq_data->chip->irq_set_irqchip_state(irq_data, IRQCHIP_STATE_PENDING, &val);

	return count;
}

static ssize_t store_irq_mask(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t store_irq_unmask(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t store_irq_pending(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static irqreturn_t gic_virtual_device_irq(int irq, void *dev_id)
{
	printk("%s\n", __func__);
	return IRQ_HANDLED;
}

static int gic_vdev_attr_init(struct virtual_gic_dev_data *data)
{
	int i;
	data->vdev_irq_attr = kzalloc(sizeof(data->vdev_irq_attr)*data->nrirqs, GFP_KERNEL);

	for (i = 0; i < data->nrirqs;i++) {
		struct gic_vdev_irq_attr *vdev_irq_attr;
		struct device_attribute  *dev_attr;
		struct attribute_group *group;
		
		vdev_irq_attr = kzalloc(sizeof(struct gic_vdev_irq_attr), GFP_KERNEL);
		sprintf((char *)vdev_irq_attr->name, "its_irq%d", data->irq + i);	

		dev_attr = &vdev_irq_attr->dev_irq_attr;
		dev_attr->store = store_irq_trigger;
		dev_attr->attr.name = IRQ_TRIGGER;
		dev_attr->attr.mode = S_IWUSR;
		vdev_irq_attr->irq_attrs[0] = &dev_attr->attr;

#if 1
		dev_attr = &vdev_irq_attr->dev_irq_mask;
		dev_attr->store = store_irq_mask;
		dev_attr->attr.name = IRQ_MASK;
		dev_attr->attr.mode = S_IWUSR;
		vdev_irq_attr->irq_attrs[1] = &dev_attr->attr;

		dev_attr = &vdev_irq_attr->dev_irq_unmask;
		dev_attr->store = store_irq_unmask;
		dev_attr->attr.name = IRQ_UNMASK;
		dev_attr->attr.mode = S_IWUSR;
		vdev_irq_attr->irq_attrs[2] = &dev_attr->attr;

		dev_attr = &vdev_irq_attr->dev_irq_pending;
		dev_attr->store = store_irq_pending;
		dev_attr->attr.name = IRQ_PENDING;
		dev_attr->attr.mode = S_IWUSR;
		vdev_irq_attr->irq_attrs[3] = &dev_attr->attr;
#endif

		vdev_irq_attr->irq = data->irq + i;

		group = &vdev_irq_attr->group;
		group->attrs = vdev_irq_attr->irq_attrs;
		group->name = vdev_irq_attr->name;

		sysfs_create_group(&data->virt_dev->kobj, group);

		data->vdev_irq_attr[i] = vdev_irq_attr;
	}
	return 0;
}

static void gic_vdev_attr_exit(struct virtual_gic_dev_data *data)
{
	int i;

	for (i = 0; i < data->nrirqs;i++) {
		struct gic_vdev_irq_attr *vdev_irq_attr =
				data->vdev_irq_attr[i];
		struct attribute_group *group = &vdev_irq_attr->group;

		sysfs_remove_group(&data->virt_dev->kobj, group);
		kfree(vdev_irq_attr);
		//kfree(group->name);
		//kfree(group);
	}
}

static struct virtual_gic_dev_data *add_new_virt_gic_dev(int nrirqs)
{
	struct virtual_gic_dev_data *data;
	msi_alloc_info_t *info;
	struct msi_desc *msi_desc;
	int irq, i, ret;
	

	data = kzalloc(sizeof(struct virtual_gic_dev_data), GFP_KERNEL);
	if (!data)
		return NULL;
	
	msi_desc = &data->msi_desc;
	info = &data->msi_info;

	data->virt_dev = virtual_get_new_device();
	if (!data->virt_dev) {
		pr_err("virtual deivice create failed\n");
		goto out;
	}

	msi_desc->dev = data->virt_dev;
	info->desc = &data->msi_desc;
	info->scratchpad[0].ul = 18;
	data->nrirqs = gic_vdev_nrirqs;

	orig_its_msi_prepare(its_domain, data->virt_dev, data->nrirqs, info);
	irq = orig___irq_domain_alloc_irqs(its_domain, -1, data->nrirqs,	
		NUMA_NO_NODE, info,false, NULL);

	if (irq >= 0) {
		data->irq = irq;
	} else {
		pr_err("virtual deivice irq create failed\n");
		goto out2;
	}
	
	//irq_data = irq_domain_get_irq_data(its_domain, irq);
	for (i = 0; i < data->nrirqs;i++) {
		ret = request_irq(irq+i, gic_virtual_device_irq, 0, "gic_vdev", NULL);
		if (ret)  {
			pr_err("virtual deivice irq register failed\n");
			goto out1;
		}
	}

	//device_create_file(data->virt_dev, &dev_attr_enable);  
	//
	dev_set_drvdata(data->virt_dev, data);

	//sysfs_create_group(&data->virt_dev->kobj, &gic_vdev_attr_group);
	gic_vdev_attr_init(data);
	return data;
out1:
	orig_irq_domain_free_irqs(irq, data->nrirqs);
out2:
	virtual_put_new_device(data->virt_dev);

out:
	kfree(data);
	return NULL;
}

static void put_new_virt_gic_dev(struct virtual_gic_dev_data *data)
{
	int i;

	//sysfs_remove_group(&data->virt_dev->kobj, &gic_vdev_attr_group);
	gic_vdev_attr_exit(data);

	for (i = 0; i < data->nrirqs;i++)
		free_irq(data->irq + i, NULL);

	orig_irq_domain_free_irqs(data->irq, data->nrirqs);
	virtual_put_new_device(data->virt_dev);
	kfree(data);
}

static struct irq_domain * get_gicv3_its_irqdomain(void)
{
	struct irq_domain *h;
	list_for_each_entry(h, orig_irq_domain_list, link) {
		if (h->ops == orig_its_domain_ops) {
			its_domain = h;
			return h;
		}
	}
	return NULL;
}

static int kallsyms_init(void)
{
	LOOKUP_SYMS(irq_domain_list);
	LOOKUP_SYMS(its_domain_ops);
	LOOKUP_SYMS(its_msi_prepare);
	LOOKUP_SYMS(__irq_domain_alloc_irqs);
	LOOKUP_SYMS(irq_domain_activate_irq);
	LOOKUP_SYMS(irq_domain_free_irqs);
	LOOKUP_SYMS(irq_domain_deactivate_irq);
	LOOKUP_SYMS(irq_domain_free_irqs);
	return 0;
}
struct virtual_gic_dev_data  *data1;

int gic_virtual_device_init(void)
{
	if (kallsyms_init())
		return -ENODEV;

	its_domain = get_gicv3_its_irqdomain();
	if (!its_domain)
		return -ENODEV;

	data1 = add_new_virt_gic_dev(1);
	return 0;
}

int gic_virtual_device_exit(void)
{
	if (data1)
		put_new_virt_gic_dev(data1);
	return 0;
}

