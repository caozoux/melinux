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

#ifdef CONFIG_ARM64
#include <linux/irqchip/arm-gic-common.h>
#include <linux/irqchip/arm-gic-v3.h>
#endif
#include "../template_iocmd.h"
#include "../misc_ioctl.h"
#include "../debug_ctrl.h"
#include "gic_local_data.h"
#include "mekernel.h"

struct rdists **orig_gic_rdists;
struct list_head *orig_its_nodes;
struct list_head *orig_lpi_range_list;
static struct gic_chip_data *orig_gic_data;

//#define gic_data_rdist()                (this_cpu_ptr(orig_gic_data->rdists.rdist))
#define gic_data_rdist()                (raw_cpu_ptr((*orig_gic_rdists)->rdist))
#define gic_data_rdist_rd_base()        (gic_data_rdist()->rd_base)
#define GIC_DATA_MAKS(val) 		 (val & ~((1<<12)-1) &  ((1ULL<<48) -1))

static void dump_data(unsigned long *addr, int size)
{
	int i;
	for (i=0; i < size; i++) {
		printk("%d:%lx\n", i, addr[i]);
	}
}

static struct its_baser *its_get_baser(struct its_node *its, u32 type)                               
{       
        int i;
        for (i = 0; i < GITS_BASER_NR_REGS; i++) {
                if (GITS_BASER_TYPE(its->tables[i].val) == type)
                        return &its->tables[i];
        }
        return NULL;
}    

static u64 its_read_baser(struct its_node *its, struct its_baser *baser)
{
	u32 idx = baser - its->tables;
	return gits_read_baser(its->base + GITS_BASER + (idx << 3));
}

static void gic_its_percpu_peng_table(void)
{
	//void __iomem *rbase = gic_data_rdist_rd_base();
	int cpu;

	for_each_possible_cpu(cpu) {
		void __iomem *rbase = per_cpu_ptr((*orig_gic_rdists)->rdist, cpu)->rd_base;
		phys_addr_t paddr = GIC_DATA_MAKS(gicr_read_pendbaser(rbase + GICR_PENDBASER));
		printk("pending  table:%lx\n", (unsigned long)paddr);
	}
}

static void gic_its_property_table(void)
{
	void *va;
	u8 *cfg;
	int i;

	//cfg = va + hwirq - 8192;
	//void __iomem *rbase = gic_data_rdist_rd_base();
	printk("property table:%llx@%llx direct_lpi:%lld\n"
		,(u64) (*orig_gic_rdists)->prop_table_pa
		,(u64) (*orig_gic_rdists)->prop_table_va
		,(u64) (*orig_gic_rdists)->has_direct_lpi);
	va = (*orig_gic_rdists)->prop_table_va;
	
	for (i=0; i< 64; i++) {
		cfg = va +i ;
		//printk("%d cfg:%x\n", i, *cfg);
	}
}

static void gic_its_virt_cpu_table(struct its_baser *baser)
{
	printk("its virtual CPUs tlb type: %lx@%lx\n", (unsigned long)baser->base,
			(unsigned long)virt_to_phys(baser->base));
}

static unsigned long gic_its_device_table_item(struct its_device *its_dev)
{
	 struct its_baser *baser;
	 struct its_node *its = its_dev->its;
	 __le64 *table;
	 u32 idx,esz;
	 u32 id = its_dev->device_id;

	 baser = its_get_baser(its, GITS_BASER_TYPE_DEVICE);
	 esz = GITS_BASER_ENTRY_SIZE(baser->val);
	 idx = id >> ilog2(baser->psz / esz);
	 table = baser->base;
#if 0
	 printk("baser:%lx table:%lx device_id:%d table:%lx idx:%d\n", baser, table[its_dev->device_id], its_dev->device_id, table, idx);
	 if (!table[its_dev->device_id])
		return -ENOMEM;
#endif
	return le64_to_cpu(table[idx]) & ~GITS_BASER_VALID;
}

static void gic_its_device_dump(struct its_device *dev)
{
	unsigned long device_table_addr = gic_its_device_table_item(dev);
	printk("its_device:%d nr_its:%d lpi_map:%llx col_map:%llx lpi_base:%llx nr_lpis:%lld\n"
			,dev->device_id
			,dev->nr_ites
			,(u64)dev->event_map.lpi_map
			,(u64)dev->event_map.col_map
			,(u64)dev->event_map.lpi_base
			,(u64)dev->event_map.nr_lpis);
	printk("             0x%lx\n", device_table_addr);
	
}

static void gic_its_device_table_summary(struct its_baser *baser)
{
	struct its_device *its_dev;
	struct its_node *its;

	list_for_each_entry(its, orig_its_nodes, entry) {
		list_for_each_entry(its_dev, &its->its_device_list, entry) {
			gic_its_device_dump(its_dev);		
		}
	}
}

static struct its_device *gic_its_device_find(int dev_id)
{
	struct its_device *its_dev;
	struct its_node *its;

	list_for_each_entry(its, orig_its_nodes, entry) {
		list_for_each_entry(its_dev, &its->its_device_list, entry) {
			if (its_dev->device_id == dev_id)
				return its_dev;
		}
	}
	return NULL;
}

static void gic_its_device_table_entry(u64 *addr)
{
	//dump_data((unsigned long)addr, 512);
	printk("addr:%lx\n", addr);
}

static void gic_its_device_table_entry_list(struct its_baser *baser)
{
	int number =0;
	u64 *addr;
	addr = (u64*) baser->base;
	for (number = 0; number < 8192; number++) {
		if (addr[number] != 0) {
			printk("addr:%lx %lx\n", addr[number], phys_to_virt(addr[number] & ~GITS_BASER_VALID));
			//gic_its_device_table_entry(phys_to_virt(addr[number] & ~GITS_BASER_VALID));
			//break;
		}
	}
		
}

static void gic_its_device_table(struct its_baser *baser)
{
	printk("its Device tlb type: %lx@%lx\n", (unsigned long)baser->base,
			(unsigned long)virt_to_phys(baser->base));
	//gic_its_device_table_summary(baser);
	gic_its_device_table_entry_list(baser);
}

static int gic_its_interrupt_mis_irq_dump(int irq)
{
	bool val = 1;
	return irq_get_irqchip_state(irq, IRQCHIP_STATE_PENDING, &val);	
}

static void gic_its_interrut_transfer(void)
{
	struct its_device *its_dev;
	its_dev = gic_its_device_find(8);
	if (its_dev) {
#if 0
		struct its_node *its = its_dev->its;
		u32 *addr;
		addr = (u32 *)ioremap(its->phys_base + GITS_TRANSLATER, PAGE_SIZE);
		iounmap(addr);
#else
		gic_its_interrupt_mis_irq_dump(45);
#endif
	}
}

static void struct_collection_dump(struct its_collection *collections)
{
#if 0
	printk("collections target_address:%llx, col_id:%llx\n"
		,collections->target_address
		,collections->col_id);
#endif
}

static void gic_its_interript_collection_table_collection(struct its_node *its,
		struct its_baser *baser)
{
	struct its_collection   *collections;
	int i;

	for (i = 0; i < nr_cpu_ids; i++) {
		collections = &its->collections[i];	
		struct_collection_dump(collections);
	}
}

static void gic_its_interript_collection_table_summar(struct its_baser *baser)
{
	int number =0;
	u64 *addr;
	addr = (u64*) baser->base;

	for (number = 0; number < 8192; number++) {
		if (addr[number] != 0) {
			printk("interrupt collection %d addr:%lx %lx\n", number, addr[number], phys_to_virt(addr[number] & ~GITS_BASER_VALID));
			//break;
		}
	}

}

static void gic_its_interript_collection_table_list(struct its_baser *baser)
{
	
}

static void gic_its_interript_collection_table(struct its_node *its, struct its_baser *baser)
{
	printk("its interrupt Collections tbl type: %lx@%lx\n", (unsigned long)baser->base,
			(unsigned long)virt_to_phys(baser->base));
	gic_its_interript_collection_table_summar(baser);
	gic_its_interript_collection_table_list(baser);
	//gic_its_interript_collection_table_collection(its, baser);
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
				gic_its_device_table(baser);
				break;
			case GITS_BASER_TYPE_VCPU:
				//gic_its_virt_cpu_table(baser);
				break;
			case GITS_BASER_TYPE_COLLECTION:
				gic_its_interript_collection_table(its, baser);
				break;
		}
	}
}

static void gic_its_base_case(struct its_node *its)
{
	unsigned long virt, phys;
	phys = gits_read_cbaser(its->base + GITS_CBASER);
}

static void gic_its_node(void)
{
	struct its_node *its;

	list_for_each_entry(its, orig_its_nodes, entry) {
		printk("its:%lx phy_base:%llx\n", (unsigned long)its, its->phys_base);
		gic_its_scan_table(its);
	}
}

static void gic_dump_register(void)
{
	gic_its_node();
	gic_its_property_table();
	//gic_its_percpu_peng_table();
}

int arm64gic_unit_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data)
{
	int ret = -1;
	switch (data->cmdcode) {
		default:
			goto OUT;
	}
OUT:
	return ret;
}

int arm64gic_unit_init(void)
{
	LOOKUP_SYMS(gic_rdists);
	LOOKUP_SYMS(its_nodes);
	LOOKUP_SYMS(gic_data);
	LOOKUP_SYMS(lpi_range_list);
	gic_dump_register();
	gic_virtual_device_init();
	//pci_bus_scan();
	return 0;
}

int arm64gic_unit_exit(void)
{
	gic_virtual_device_exit();
	return 0;
}

