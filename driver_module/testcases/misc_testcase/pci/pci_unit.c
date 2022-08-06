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
	struct pcidevice_config_data pci_data;
	pci_read_config_word(pdev, PCI_VENDOR_ID, &pci_data.vendor);
	pci_read_config_word(pdev, PCI_DEVICE_ID, &pci_data.device);
	pci_read_config_word(pdev, PCI_COMMAND, &pci_data.command);
	pci_read_config_word(pdev, PCI_STATUS, &pci_data.status);

	pci_read_config_dword(pdev, PCI_CLASS_REVISION, &pci_data.class_code);
	pci_read_config_byte(pdev, PCI_CACHE_LINE_SIZE, &pci_data.cache_line);
	pci_read_config_byte(pdev, PCI_LATENCY_TIMER, &pci_data.lat_timer);
	pci_read_config_byte(pdev, PCI_HEADER_TYPE, &pci_data.header_type);
	pci_read_config_byte(pdev, PCI_BIST, &pci_data.bist);
	pci_read_config_dword(pdev, PCI_BASE_ADDRESS_0, &pci_data.resource0);
	pci_read_config_dword(pdev, PCI_BASE_ADDRESS_1, &pci_data.resource1);
	pci_read_config_dword(pdev, PCI_BASE_ADDRESS_2, &pci_data.resource2);
	pci_read_config_dword(pdev, PCI_BASE_ADDRESS_3, &pci_data.resource3);
	pci_read_config_dword(pdev, PCI_BASE_ADDRESS_4, &pci_data.resource4);
	pci_read_config_dword(pdev, PCI_BASE_ADDRESS_5, &pci_data.resource5);
	pci_read_config_dword(pdev, PCI_CARDBUS_CIS, &pci_data.cardbus_pointer);
	pci_read_config_word(pdev, PCI_SUBSYSTEM_VENDOR_ID, &pci_data.subvendor);
	pci_read_config_word(pdev, PCI_SUBSYSTEM_ID, &pci_data.subid);
	pci_read_config_dword(pdev, PCI_ROM_ADDRESS, &pci_data.rom_baseaddr);
	pci_read_config_word(pdev, PCI_CAPABILITY_LIST, &pci_data.cap_pointer);
	pci_read_config_byte(pdev, PCI_INTERRUPT_LINE, &pci_data.interrupt_line);
	pci_read_config_byte(pdev, PCI_INTERRUPT_PIN, &pci_data.interrupt_pin);
	pci_read_config_byte(pdev, PCI_MIN_GNT, &pci_data.min_gnt);
	pci_read_config_byte(pdev, PCI_MAX_LAT, &pci_data.max_lat);

	printk("vendor:%x device:%x \n",pci_data.vendor, pci_data.device);
	printk("command:%x status:%x class code:%x \n",pci_data.command, pci_data.status, pci_data.class_code);
	printk("bist:%x HeadType:%x Lantcy_timer:%x cache_line_size:%x\n",
			pci_data.bist, pci_data.header_type,
			pci_data.lat_timer, pci_data.cache_line);

	printk("resource0:%08x resource1:%08x resource2:%08x \n",
			 pci_data.resource0, pci_data.resource1, pci_data.resource2);
	printk("resource3:%08x resource4:%08x resource5:%08x \n",
			 pci_data.resource3, pci_data.resource4, pci_data.resource5);
	printk("cardbus:%x subvendor:%x subid:%x \n", pci_data.cardbus_pointer
			,pci_data.subvendor, pci_data.subid);
	printk("rom base:%x capablity list:%x\n", pci_data.rom_baseaddr, pci_data.cap_pointer);
	printk("interrupt_pin:%x interrupt_line:%x min gnt:%x max lat:%x \n",
			pci_data.interrupt_pin, pci_data.interrupt_line
			, pci_data.min_gnt,pci_data.max_lat);
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
			printk("res:[%lx-%lx]\n",(unsigned long)res->start, (unsigned long)res->end);
	}
	read_pci_config_space(pdev);
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

