#include <linux/init.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/notifier.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/timekeeper_internal.h>
#include <kernel/sched/sched.h>
#include <linux/pci.h>

#include <asm/stacktrace.h>
#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "pcilocal.h"
#include "debug_ctrl.h"
#include "medelay.h"


// read the config space data from pdev
static void read_pci_config_space(struct pci_dev *pdev)
{
	
}

static void pci_dev_data_dump(struct pci_dev *pdev)
{
	int i;
	printk("devfn:%ld vendor:%lx device:%lx class:%lx aer_cap:%lx irq:%ld \n",
				(unsigned long)pdev->devfn,
				(unsigned long)pdev->vendor,
				(unsigned long)pdev->device,
				(unsigned long)pdev->class,
				(unsigned long)pdev->aer_cap,
				(unsigned long)pdev->irq);
	for (i = 0; i < DEVICE_COUNT_RESOURCE; ++i) {
		struct resource *res = &pdev->resource[i];
		if (res->start)
			printk("zz %s start:%lx end:%lx \n",__func__, (unsigned long)res->start, (unsigned long)res->end);
	}
}

#if 0
static void pci_unit_find_device(void)
{

}
#endif

static void pci_unit_enumalte_all(void)
{
	struct pci_dev *pdev;
	struct device  *dev;
	struct pci_bus *bus = NULL;

	while ((bus = pci_find_next_bus(bus)) != NULL)  {
		list_for_each_entry(pdev, &bus->devices, bus_list) {
			dev = &pdev->dev;
			printk("pci:%s parent:%s\n", dev_name(dev), dev_name(dev->parent));
			pci_dev_data_dump(pdev);
		}
	}
}

int pci_unit_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_data *data)
{
	switch (data->cmdcode) {
		case IOCTL_PCI_ENUM:
			pci_unit_enumalte_all();
			break;
		default:
			break;
	}

	return 0;
}

int pci_unit_init(void)
{
	return 0;
}

int pci_unit_exit(void)
{
	return 0;
}

