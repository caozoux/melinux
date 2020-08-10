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

#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"

struct bus_type *orig_pci_bus_type;

static int __must_check pci_rescan_devices(struct device *dev, void *data)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	int *addr;

	if (pdev->msix_enabled) {
		 struct msi_desc *entry;
		 for_each_pci_msi_entry(entry, pdev) {
			addr = (int *)ioremap(entry->msg.address_lo, PAGE_SIZE);
			printk("msi irq:%d addr_ld:%x add_hd:%x data:%x\n", pdev->irq, entry->msg.address_lo, entry->msg.address_lo, *addr);
			iounmap(addr);
			break;
		 }	
	}
	printk("zz %s %d %s\n", __func__, __LINE__, dev_name(dev));
	//for_each_pci_msi_entry
	return 0;
}

int hwpci_unit_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data)
{
	int ret = -1;
	switch (data->cmdcode) {
		default:
			goto OUT;
	}
OUT:
	return ret;
}

int hwpci_unit_init(void)
{
	LOOKUP_SYMS(pci_bus_type);
	bus_for_each_dev(orig_pci_bus_type, NULL, NULL, pci_rescan_devices);
	return 0;
}

int hwpci_unit_exit(void)
{
	return 0;
}

