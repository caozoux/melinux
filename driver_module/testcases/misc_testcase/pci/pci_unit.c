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


static void pci_unit_enumalte_all(void)
{
	struct pci_dev *pdev;
	struct device  *dev;
	struct pci_bus *bus = NULL;
	struct pci_bus *tmp_bus;

	while ((bus = pci_find_next_bus(bus)) != NULL)  {
		list_for_each_entry(pdev, &bus->devices, bus_list) {
			dev = &pdev->dev;
			printk("pci:%s parent:%s\n", dev_name(dev), dev_name(dev->parent));
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

