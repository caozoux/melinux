#include <linux/aer.h>
#include <linux/async.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>
#include <linux/blk-mq-pci.h>
#include <linux/dmi.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/once.h>
#include <linux/pci.h>
#include <linux/t10-pi.h>
#include <linux/types.h>
#include <linux/io-64-nonatomic-lo-hi.h>
#include <linux/sed-opal.h>

#include "nvme.h"

#define DEBUG

struct nvme_pdev {
	struct device *dev;
	struct pci_dev *pdev;
	void __iomem *iomem;
	u32 __iomem *iomem_dbs;
	struct nvme_ctrl ctrl;
};

static inline struct nvme_pdev *to_nvme_dev(struct nvme_ctrl *ctrl)
{
	return container_of(ctrl, struct nvme_pdev, ctrl);
}

static void nvme_pci_submit_async_event(struct nvme_ctrl *ctrl)
{
#if 0
	struct nvme_dev *dev = to_nvme_dev(ctrl);
	struct nvme_queue *nvmeq = &dev->queues[0];
	struct nvme_command c;

	memset(&c, 0, sizeof(c));
	c.common.opcode = nvme_admin_async_event;
	c.common.command_id = NVME_AQ_BLK_MQ_DEPTH;
	nvme_submit_cmd(nvmeq, &c);
#endif
}

static void nvme_pci_free_ctrl(struct nvme_ctrl *ctrl)
{
#if 0
	struct nvme_dev *dev = to_nvme_dev(ctrl);

	nvme_dbbuf_dma_free(dev);
	put_device(dev->dev);
	if (dev->tagset.tags)
		blk_mq_free_tag_set(&dev->tagset);
	if (dev->ctrl.admin_q)
		blk_put_queue(dev->ctrl.admin_q);
	kfree(dev->queues);
	free_opal_dev(dev->ctrl.opal_dev);
	mempool_destroy(dev->iod_mempool);
	kfree(dev);
#endif
}

static int nvme_pci_reg_read32(struct nvme_ctrl *ctrl, u32 off, u32 *val)
{
	*val = readl(to_nvme_dev(ctrl)->iomem+ off);
	return 0;
}

static int nvme_pci_reg_write32(struct nvme_ctrl *ctrl, u32 off, u32 val)
{
	writel(val, to_nvme_dev(ctrl)->iomem  + off);
	return 0;
}

static int nvme_pci_reg_read64(struct nvme_ctrl *ctrl, u32 off, u64 *val)
{
	*val = readq(to_nvme_dev(ctrl)->iomem  + off);
	return 0;
}

static int nvme_pci_get_address(struct nvme_ctrl *ctrl, char *buf, int size)
{
	struct pci_dev *pdev = to_pci_dev(to_nvme_dev(ctrl)->dev);

	return snprintf(buf, size, "%s", dev_name(&pdev->dev));
}
static const struct nvme_ctrl_ops nvme_pci_ctrl_ops = {
	.name			= "pcie",
	.module			= THIS_MODULE,
	.flags			= NVME_F_METADATA_SUPPORTED,
	.reg_read32		= nvme_pci_reg_read32,
	.reg_write32		= nvme_pci_reg_write32,
	.reg_read64		= nvme_pci_reg_read64,
	.free_ctrl		= nvme_pci_free_ctrl,
	.submit_async_event	= nvme_pci_submit_async_event,
	.get_address		= nvme_pci_get_address,
};

static void nvme_reset_work(struct work_struct *work)
{
	printk("zz %s %d \n", __func__, __LINE__);
}

static void nvme_reset_prepare(struct pci_dev *pdev)
{
}

static void nvme_reset_done(struct pci_dev *pdev)
{
}

static pci_ers_result_t nvme_error_detected(struct pci_dev *pdev,
						pci_channel_state_t state)
{
	return PCI_ERS_RESULT_NEED_RESET;
}

static pci_ers_result_t nvme_slot_reset(struct pci_dev *pdev)
{
	return PCI_ERS_RESULT_RECOVERED;
}

static void nvme_error_resume(struct pci_dev *pdev)
{
}

static int nvme_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{

	struct nvme_pdev *nvme_pdev;
	int node;
	int ret = 0;
	int size = NVME_REG_DBS + 4096;

	printk("zz %s pdev:%lx \n",__func__, (unsigned long)pdev);
	node = dev_to_node(&pdev->dev);
	if (node == NUMA_NO_NODE)
		set_dev_node(&pdev->dev, first_memory_node);

	nvme_pdev = kzalloc_node(sizeof(nvme_pdev), GFP_KERNEL, node);

	if (!nvme_pdev)
		return -EINVAL;

	nvme_pdev->pdev = pdev;
	nvme_pdev->dev = get_device(&pdev->dev);

	pci_set_drvdata(pdev, nvme_pdev);

	/*
	 * request resource ranage, it is unique, you can see 
	 * it in /proc/iomem
	 */
	if (pci_request_mem_regions(pdev, "nvme_retest"))
		return -ENODEV;

	nvme_pdev->iomem = ioremap(pci_resource_start(pdev, 0), size);
	if (!nvme_pdev->iomem) {
		ret = -ENOMEM;
		goto release;
	}

	nvme_pdev->iomem_dbs = nvme_pdev->iomem + NVME_REG_DBS;

	//nvme_reset_work will be callback by nvme_init_ctrl
	INIT_WORK(&nvme_pdev->ctrl.reset_work, nvme_reset_work);

#if 0
	if (nvme_remap_bar(dev, NVME_REG_DBS + 4096))
		goto release;
#else
	printk("zz %s size:%lx addr:%lx\n",__func__, (unsigned long)pci_resource_len(pdev, 0), pci_resource_start(pdev, 0));
	printk("zz %s iomem:%lx iomem_dbs:%lx \n",__func__, (unsigned long)nvme_pdev->iomem, (unsigned long)nvme_pdev->iomem_dbs);
#endif

	return 0;

release:
	pci_release_mem_regions(pdev);
	return -ENODEV;
}

static void nvme_remove(struct pci_dev *pdev)
{
	struct nvme_pdev *nvme_pdev = pci_get_drvdata(pdev);

	pci_set_drvdata(pdev, NULL);

	if (nvme_pdev->iomem)
		iounmap(nvme_pdev->iomem);

	pci_release_mem_regions(to_pci_dev(nvme_pdev->dev));
	kfree(nvme_pdev);
}

static void nvme_shutdown(struct pci_dev *pdev)
{
	//struct nvme_dev *dev = pci_get_drvdata(pdev);
	//nvme_dev_disable(dev, true);
}

static const struct pci_error_handlers nvme_err_handler = {
	.error_detected	= nvme_error_detected,
	.slot_reset	= nvme_slot_reset,
	.resume		= nvme_error_resume,
	.reset_prepare	= nvme_reset_prepare,
	.reset_done	= nvme_reset_done,
};

static const struct pci_device_id nvme_id_table[] = {
	{ PCI_VDEVICE(INTEL, 0x0953),
		.driver_data = NVME_QUIRK_STRIPE_SIZE |
				NVME_QUIRK_DEALLOCATE_ZEROES, },
	{ PCI_VDEVICE(INTEL, 0x0a53),
		.driver_data = NVME_QUIRK_STRIPE_SIZE |
				NVME_QUIRK_DEALLOCATE_ZEROES, },
	{ PCI_VDEVICE(INTEL, 0x0a54),
		.driver_data = NVME_QUIRK_STRIPE_SIZE |
				NVME_QUIRK_DEALLOCATE_ZEROES, },
	{ PCI_VDEVICE(INTEL, 0x0a55),
		.driver_data = NVME_QUIRK_STRIPE_SIZE |
				NVME_QUIRK_DEALLOCATE_ZEROES, },
	{ PCI_VDEVICE(INTEL, 0xf1a5),	/* Intel 600P/P3100 */
		.driver_data = NVME_QUIRK_NO_DEEPEST_PS |
				NVME_QUIRK_MEDIUM_PRIO_SQ },
	{ PCI_VDEVICE(INTEL, 0x5845),	/* Qemu emulated controller */
		.driver_data = NVME_QUIRK_IDENTIFY_CNS, },
	{ PCI_DEVICE(0x1bb1, 0x0100),   /* Seagate Nytro Flash Storage */
		.driver_data = NVME_QUIRK_DELAY_BEFORE_CHK_RDY, },
	{ PCI_DEVICE(0x1c58, 0x0003),	/* HGST adapter */
		.driver_data = NVME_QUIRK_DELAY_BEFORE_CHK_RDY, },
	{ PCI_DEVICE(0x1c58, 0x0023),	/* WDC SN200 adapter */
		.driver_data = NVME_QUIRK_DELAY_BEFORE_CHK_RDY, },
	{ PCI_DEVICE(0x1c5f, 0x0540),	/* Memblaze Pblaze4 adapter */
		.driver_data = NVME_QUIRK_DELAY_BEFORE_CHK_RDY, },
	{ PCI_DEVICE(0x144d, 0xa821),   /* Samsung PM1725 */
		.driver_data = NVME_QUIRK_DELAY_BEFORE_CHK_RDY, },
	{ PCI_DEVICE(0x144d, 0xa822),   /* Samsung PM1725a */
		.driver_data = NVME_QUIRK_DELAY_BEFORE_CHK_RDY, },
	{ PCI_DEVICE(0x1d1d, 0x1f1f),	/* LighNVM qemu device */
		.driver_data = NVME_QUIRK_LIGHTNVM, },
	{ PCI_DEVICE(0x1d1d, 0x2807),	/* CNEX WL */
		.driver_data = NVME_QUIRK_LIGHTNVM, },
	{ PCI_DEVICE(0x1d1d, 0x2601),	/* CNEX Granby */
		.driver_data = NVME_QUIRK_LIGHTNVM, },
	{ PCI_DEVICE_CLASS(PCI_CLASS_STORAGE_EXPRESS, 0xffffff) },
	{ PCI_DEVICE(PCI_VENDOR_ID_APPLE, 0x2001) },
	{ PCI_DEVICE(PCI_VENDOR_ID_APPLE, 0x2003) },
	{ 0, }
};
MODULE_DEVICE_TABLE(pci, nvme_id_table);

static struct pci_driver nvme_driver = {
	.name		= "nvme_retest",
	.id_table	= nvme_id_table,
	.probe		= nvme_probe,
	.remove		= nvme_remove,
	.shutdown	= nvme_shutdown,
	/*
	.driver		= {
		.pm	= &nvme_dev_pm_ops,
	},
	*/
	.sriov_configure = pci_sriov_configure_simple,
	.err_handler	= &nvme_err_handler,
};

static int __init nvme_init(void)
{
	return pci_register_driver(&nvme_driver);
}

static void __exit nvme_exit(void)
{
	pci_unregister_driver(&nvme_driver);
}

MODULE_AUTHOR("Matthew Wilcox <willy@linux.intel.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
module_init(nvme_init);
module_exit(nvme_exit);
