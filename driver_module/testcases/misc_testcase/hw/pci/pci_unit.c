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

#ifdef CONFIG_ARM64
#include <linux/irqchip/arm-gic-common.h>
#include <linux/irqchip/arm-gic-v3.h>
#endif
#include "../../template_iocmd.h"
#include "../../misc_ioctl.h"
#include "../../debug_ctrl.h"

struct bus_type *orig_pci_bus_type;
struct bus_type *orig_virtio_bus;

static int __must_check pci_rescan_devices(struct device *dev, void *data)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	unsigned long *addr;
	unsigned long ali_addr;

	if (pdev->msix_enabled) {
		 struct msi_desc *entry;
		 for_each_pci_msi_entry(entry, pdev) {
			ali_addr = (entry->msg.address_lo&0x1000);
			addr = (unsigned long *)ioremap(ali_addr, PAGE_SIZE);
			iounmap(addr);
		 }	
	}
	return 0;
}

int hwpci_unit_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data)
{
	int ret = -1;
	switch (data->cmdcode) {
		default:
			bus_for_each_dev(orig_pci_bus_type, NULL, NULL, pci_rescan_devices);
			goto OUT;
	}
OUT:
	return ret;
}

int hwpci_unit_init(void)
{
	LOOKUP_SYMS(pci_bus_type);
	return 0;
}

int hwpci_unit_exit(void)
{
	return 0;
}

