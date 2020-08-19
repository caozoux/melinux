#ifndef __GIC_LOCAL_DATA_H__
#define __GIC_LOCAL_DATA_H__
#include <linux/version.h>

struct lpi_range { 
	struct list_head        entry;
	u32                     base_id;
	u32                     span; 
};

struct redist_region {
        void __iomem            *redist_base;
        phys_addr_t             phys_base;
        bool                    single_redist;
};

struct gic_chip_data {
        struct fwnode_handle    *fwnode;
        void __iomem            *dist_base;
        struct redist_region    *redist_regions;
        struct rdists           rdists;
        struct irq_domain       *domain;
        u64                     redist_stride;
        u32                     nr_redist_regions;
        u64                     flags;
        bool                    has_rss;
        unsigned int            ppi_nr;
        struct partition_desc   **ppi_descs;
};

struct its_collection {
        u64                     target_address;
        u16                     col_id;
};

/*
 *  * The ITS_BASER structure - contains memory information, cached
 *   * value of BASER register configuration and ITS page size.
 *    */
struct its_baser {
        void            *base;
        u64             val;
        u32             order;
        u32             psz;
};

struct its_node {
        raw_spinlock_t          lock;
        struct mutex            dev_alloc_lock;
        struct list_head        entry;
        void __iomem            *base;
        void __iomem            *sgir_base;
        phys_addr_t             phys_base;
        struct its_cmd_block    *cmd_base;
        struct its_cmd_block    *cmd_write;
        struct its_baser        tables[GITS_BASER_NR_REGS];
        struct its_collection   *collections;
        struct fwnode_handle    *fwnode_handle;
        u64                     (*get_msi_base)(struct its_device *its_dev);
        u64                     typer;
        u64                     cbaser_save;
        u32                     ctlr_save;
        u32                     mpidr;
        struct list_head        its_device_list;
        u64                     flags;
        unsigned long           list_nr;
        int                     numa_node;
        unsigned int            msi_domain_flags;
        u32                     pre_its_base; /* for Socionext Synquacer */
        int                     vlpi_redist_offset;
};


//#if LINUX_VERSION_CODE < KERNEL_VERSION(4,12,0)
#if LINUX_VERSION_CODE <  KERNEL_VERSION(5,0,0)
struct event_lpi_map {
        unsigned long           *lpi_map;
        u16                     *col_map;
        irq_hw_number_t         lpi_base;
        int                     nr_lpis;
        struct mutex            vlpi_lock;
        struct its_vm           *vm;
        struct its_vlpi_map     *vlpi_maps;
        int                     nr_vlpis;
};
#else
struct event_lpi_map {
        unsigned long           *lpi_map;
        u16                     *col_map;
        irq_hw_number_t         lpi_base;
        int                     nr_lpis;
	raw_spinlock_t          vlpi_lock;
        struct its_vm           *vm;
        struct its_vlpi_map     *vlpi_maps;
        int                     nr_vlpis;
};
#endif

struct its_device {
        struct list_head        entry;
        struct its_node 	*its;
        struct event_lpi_map    event_map;
        void                    *itt;
        u32                     nr_ites;
        u32                     device_id;
        bool                    shared;
};

int gic_virtual_device_init(void);
int gic_virtual_device_exit(void);

#endif
