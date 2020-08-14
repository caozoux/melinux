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
#include "../../gic_local_data.h"

struct bus_type *orig_pci_bus_type;
struct bus_type *orig_virtio_bus;
struct rdists **orig_gic_rdists;
struct list_head *orig_its_nodes;
static struct gic_chip_data *orig_gic_data;


//#define gic_data_rdist()                (this_cpu_ptr(orig_gic_data->rdists.rdist))
#define gic_data_rdist()                (raw_cpu_ptr((*orig_gic_rdists)->rdist))
#define gic_data_rdist_rd_base()        (gic_data_rdist()->rd_base)
#define GIC_DATA_MAKS(val) 		 (val & ~((1<<12)-1) &  ((1ULL<<48) -1))

static u64 its_read_baser(struct its_node *its, struct its_baser *baser)
{
	u32 idx = baser - its->tables;
	return gits_read_baser(its->base + GITS_BASER + (idx << 3));
}

static void gic_rdists_property_table(void)
{
	void __iomem *rbase = gic_data_rdist_rd_base();
	printk("property table:%llx\n", (*orig_gic_rdists)->prop_table_pa);
	printk("pending  table:%llx\n", GIC_DATA_MAKS(gicr_read_pendbaser(rbase + GICR_PENDBASER)));
}

static void gic_its_virt_cpu_table(void)
{
	printk("its virtual CPUs tlb type\n");
}

static void gic_its_device_table(void)
{
	printk("its Device tlb type\n");
}

static void gic_its_interript_collection_table(void)
{
	printk("its interrupt Collections tbl type\n");
}

static void gic_its_scan_table(struct its_node *its)
{
	int i;
	struct its_baser *baser;
	u64 type, val;

	for (i = 0; i < GITS_BASER_NR_REGS; i++)  {
		baser = its->tables + i;
		val = its_read_baser(its, baser);
		type = GITS_BASER_TYPE(val);
		switch (type) {
			case GITS_BASER_TYPE_DEVICE:
				gic_its_device_table();
				break;
			case GITS_BASER_TYPE_VCPU:
				gic_its_virt_cpu_table();
				break;
			case GITS_BASER_TYPE_COLLECTION:
				gic_its_interript_collection_table();
				break;
		}
	}
}

static void gic_its_node(void)
{
#if 0
	struct its_node *its;
	list_for_each_entry(its, orig_its_nodes, entry) {
		printk("its:%p idx%d:%lx \n", its, 0,
			 gits_read_baser((its->base + GITS_BASER)) & ~((1<<12)-1) &  ((1ULL<<48) -1));
	}
#else
	struct its_node *its;

	list_for_each_entry(its, orig_its_nodes, entry) {
		printk("its:%lx phy_base:%lx\n", its, its->phys_base);
		gic_its_scan_table(its);
	}
#endif
}

static void gic_dump_register(void)
{
#if 0
#else
	//printk("SETLPIR:%llx\n", (u64)readl_relaxed(rbase + GICR_SETLPIR));
	gic_its_node();
	gic_rdists_property_table();
	//writel_relaxed(52, rbase + GICR_SETLPIR);
#endif
}

static void gic_lts(int irq)
{
	struct irq_data *d = irq_get_irq_data(irq);
	struct its_device *its_dev = irq_data_get_irq_chip_data(d);

	printk("%s %p its_dev:%p hwirq:%ld\n", __func__, d, its_dev, d->hwirq);
	if (its_dev) {
		printk("lpi_map:%llx col_map:%llx lpi_base:%llx\n"
							, (u64) its_dev->event_map.lpi_map
							, (u64) its_dev->event_map.col_map
							, (u64) its_dev->event_map.lpi_base);
	}
}

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
#if 0
			if (entry->irq == 389) {
				//struct msi_msg msg;
				//*addr = 0xffffffff;
				//addr[8] = 0xffffff;
				//printk("msi irq:%d addr_ld:%x add_hd:%x data:%x\n", entry->irq, ali_addr, entry->msg.address_lo, *addr);
				//printk("1:%lx 2:%lx 3:%lx 4:%lx  5:%lx %lx\n", addr[0], addr[1], addr[2], addr[3], addr[8], &addr[8]);
				 //get_cached_msi_msg(389, &msg);
				 //pci_write_msi_msg(389, &msg);
			}
#else
				//gic_lts(entry->irq);
				//struct its_device *its_dev = irq_data_get_irq_chip_data(d);
#if 0
				int hwirq;
				void *va;
				u8 *cfg;
				hwirq = d->hwirq;
				cfg = va + hwirq - 8192;
				printk("irq:%d hwirq:%d cfg:%x\n", entry->irq, hwirq, *cfg);
#endif

#endif
			iounmap(addr);
		 }	
	}
	//printk("zz %s %d %s\n", __func__, __LINE__, dev_name(dev));
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
	LOOKUP_SYMS(gic_rdists);
	LOOKUP_SYMS(its_nodes);
	LOOKUP_SYMS(gic_data);
	gic_dump_register();
#if 0
	LOOKUP_SYMS(virtio_bus);
	bus_for_each_dev(orig_virtio_bus, NULL, NULL, pci_rescan_devices);
#endif
	bus_for_each_dev(orig_pci_bus_type, NULL, NULL, pci_rescan_devices);
	return 0;
}

int hwpci_unit_exit(void)
{
	return 0;
}

