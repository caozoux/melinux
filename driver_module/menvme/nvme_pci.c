#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/io.h>
#include <linux/io-64-nonatomic-lo-hi.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/blk-mq-pci.h>
#include <linux/pci.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>
#include <linux/blk-mq-pci.h>
#include <linux/aer.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/irqdomain.h>

#include "menvme.h"

static int io_queue_depth_set(const char *val, const struct kernel_param *kp);

#define NVME_QUIRK_IDENTIFY_CNS      (1 << 1)

static int use_threaded_interrupts;
module_param(use_threaded_interrupts, int, 0);

bool use_cmb_sqes = true;
module_param(use_cmb_sqes, bool, 0444);
MODULE_PARM_DESC(use_cmb_sqes, "use controller's memory buffer for I/O SQes");

unsigned int sgl_threshold = SZ_32K;
module_param(sgl_threshold, uint, 0644);
MODULE_PARM_DESC(sgl_threshold,
		"Use SGLs when average request segment size is larger or equal to "
		"this size. Use 0 to disable SGLs.");
static const struct kernel_param_ops io_queue_depth_ops = {
	.set = io_queue_depth_set,
	.get = param_get_int,
};

int io_queue_depth = 1024;
module_param_cb(io_queue_depth, &io_queue_depth_ops, &io_queue_depth, 0644);
MODULE_PARM_DESC(io_queue_depth, "set io queue depth, should >= 2");

static ssize_t nvme_cmb_show(struct device *dev,
			     struct device_attribute *attr,
			     char *buf)
{
	struct nvme_dev *ndev = to_nvme_dev(dev_get_drvdata(dev));

	return scnprintf(buf, PAGE_SIZE, "cmbloc : x%08x\ncmbsz  : x%08x\n",
		       ndev->cmbloc, ndev->cmbsz);
}
static DEVICE_ATTR(cmb, S_IRUGO, nvme_cmb_show, NULL);
static u64 nvme_cmb_size_unit(struct nvme_dev *dev)
{
	u8 szu = (dev->cmbsz >> NVME_CMBSZ_SZU_SHIFT) & NVME_CMBSZ_SZU_MASK;

	return 1ULL << (12 + 4 * szu);
}

static u32 nvme_cmb_size(struct nvme_dev *dev)
{
	return (dev->cmbsz >> NVME_CMBSZ_SZ_SHIFT) & NVME_CMBSZ_SZ_MASK;
}

static int io_queue_depth_set(const char *val, const struct kernel_param *kp)
{
	int n = 0, ret;

	ret = kstrtoint(val, 10, &n);
	if (ret != 0 || n < 2)
		return -EINVAL;

	return param_set_int(val, kp);
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

static void nvme_reset_prepare(struct pci_dev *pdev)
{
}

static void nvme_reset_done(struct pci_dev *pdev)
{
}

static void nvme_shutdown(struct pci_dev *pdev)
{

}

static void nvme_pci_map_cmb(struct nvme_dev *dev)
{
	u64 size, offset;
	resource_size_t bar_size;
	struct pci_dev *pdev = to_pci_dev(dev->dev);
	int bar;

	if (dev->cmb_size)
		return;

	dev->cmbsz = readl(dev->bar + NVME_REG_CMBSZ);
	if (!dev->cmbsz)
		return;
	dev->cmbloc = readl(dev->bar + NVME_REG_CMBLOC);

	if (!use_cmb_sqes)
		return;

	size = nvme_cmb_size_unit(dev) * nvme_cmb_size(dev);
	offset = nvme_cmb_size_unit(dev) * NVME_CMB_OFST(dev->cmbloc);
	bar = NVME_CMB_BIR(dev->cmbloc);
	bar_size = pci_resource_len(pdev, bar);

	if (offset > bar_size)
		return;

	/*
	 * Controllers may support a CMB size larger than their BAR,
	 * for example, due to being behind a bridge. Reduce the CMB to
	 * the reported size of the BAR
	 */
	if (size > bar_size - offset)
		size = bar_size - offset;

	dev->cmb = ioremap_wc(pci_resource_start(pdev, bar) + offset, size);
	if (!dev->cmb)
		return;
	dev->cmb_bus_addr = pci_bus_address(pdev, bar) + offset;
	dev->cmb_size = size;

	if (sysfs_add_file_to_group(&dev->ctrl.device->kobj,
				    &dev_attr_cmb.attr, NULL))
		dev_warn(dev->ctrl.device,
			 "failed to add sysfs attribute for CMB\n");
}

static inline void nvme_pci_release_cmb(struct nvme_dev *dev)
{
	if (dev->cmb) {
		iounmap(dev->cmb);
		dev->cmb = NULL;
		sysfs_remove_file_from_group(&dev->ctrl.device->kobj,
					     &dev_attr_cmb.attr, NULL);
		dev->cmbsz = 0;
	}
}

static irqreturn_t nvme_pci_irq(int irq, void *data)
{
	return nvme_irq(irq,data);
}

static irqreturn_t nvme_pci_irq_check(int irq, void *data)
{
	return nvme_irq_check(irq, data);
}

int nvme_pci_queue_request_irq(struct nvme_queue *nvmeq)
{
	struct pci_dev *pdev = to_pci_dev(nvmeq->dev->dev);
	int nr = nvmeq->dev->ctrl.instance;

	printk("zz %s %d vector:%d irq:%d %d\n", __func__, __LINE__, 
			nvmeq->cq_vector,
			pdev->irq,
			pci_irq_vector(pdev, nvmeq->cq_vector));
	if (use_threaded_interrupts) {
		return pci_request_irq(pdev, nvmeq->cq_vector, nvme_pci_irq_check,
				nvme_pci_irq, nvmeq, "nvme%dq%d", nr, nvmeq->qid);
	} else {
		return pci_request_irq(pdev, nvmeq->cq_vector, nvme_pci_irq,
				NULL, nvmeq, "nvme%dq%d", nr, nvmeq->qid);
	}
}

static int nvme_pci_init(struct pci_dev *pdev)
{
	struct nvme_dev *dev = pci_get_drvdata(pdev);
	int result = -ENOMEM;

	if (pci_enable_device_mem(pdev))
		return result;

	pci_set_master(pdev);

	if (dma_set_mask_and_coherent(dev->dev, DMA_BIT_MASK(64)) &&
	    dma_set_mask_and_coherent(dev->dev, DMA_BIT_MASK(32)))
		goto disable;

	if (readl(dev->bar + NVME_REG_CSTS) == -1) {
		result = -ENODEV;
		goto disable;
	}

	result = pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_ALL_TYPES);
	if (result < 0)
		return result;
	printk("zz %s %d irq:%d %d pdev->msix_enabled:%d\n", __func__, __LINE__,
			pdev->irq, pci_irq_vector(pdev, 0), result, pdev->msix_enabled);

	dev->ctrl.cap = lo_hi_readq(dev->bar + NVME_REG_CAP);

	dev->q_depth = min_t(int, NVME_CAP_MQES(dev->ctrl.cap) + 1,
				io_queue_depth);
	dev->db_stride = 1 << NVME_CAP_STRIDE(dev->ctrl.cap);
	dev->dbs = dev->bar + 4096;
	nvme_pci_map_cmb(dev);

	pci_enable_pcie_error_reporting(pdev);
	pci_save_state(pdev);
	return 0;

 disable:
	pci_disable_device(pdev);
	return result;
}

static void nvme_remove(struct pci_dev *pdev)
{
	struct nvme_dev *dev = pci_get_drvdata(pdev);
	struct nvme_queue *nvmeq;

	nvmeq = &dev->queues[0];

    pci_free_irq(pdev, 0, nvmeq);
	pci_free_irq_vectors(pdev);

	menvme_free_ctrl(dev);
	kfree(dev->queues);

	if (dev->bar)
		iounmap(dev->bar);
	pci_release_mem_regions(to_pci_dev(dev->dev));

	nvme_pci_release_cmb(dev);

	if (pci_is_enabled(pdev)) {
		pci_disable_pcie_error_reporting(pdev);
		pci_disable_device(pdev);
	}

	put_device(dev->dev);
	kfree(dev);
}

static int nvme_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	int node, result = -ENOMEM;
	struct nvme_dev *dev;
	unsigned long quirks = id->driver_data;
	size_t alloc_size;

	node = dev_to_node(&pdev->dev);
	if (node == NUMA_NO_NODE)
		set_dev_node(&pdev->dev, first_memory_node);

	if (node == NUMA_NO_NODE)
		set_dev_node(&pdev->dev, first_memory_node);

	dev = kzalloc_node(sizeof(*dev), GFP_KERNEL, node);
	if (!dev)
		return -ENOMEM;

	dev->queues = kcalloc_node(num_possible_cpus() + 1,
			sizeof(struct nvme_queue), GFP_KERNEL, node);
	if (!dev->queues) {
		kfree(dev);
		return -ENOMEM;
	}

	if (pci_request_mem_regions(pdev, "nvme"))
		goto free;

	dev->bar = ioremap(pci_resource_start(pdev, 0), NVME_REG_DBS + 4096);
	dev->bar_mapped_size = NVME_REG_DBS + 4096;
	dev->dbs = dev->bar + NVME_REG_DBS;

	dev->dev = get_device(&pdev->dev);
	pci_set_drvdata(pdev, dev);

	printk("zz %s dev->bar:%lx \n",__func__, (unsigned long)dev->bar);
	nvme_pci_init(pdev);
	nvme_pci_map_cmb(dev);
	menvme_init_ctrl(dev, quirks);

	return 0;

 free:
	kfree(dev->queues);
	kfree(dev);
	return result;
}

static const struct pci_error_handlers nvme_err_handler = {
    .error_detected = nvme_error_detected,
    .slot_reset = nvme_slot_reset,
    .resume     = nvme_error_resume,
    .reset_prepare  = nvme_reset_prepare,
    .reset_done = nvme_reset_done,
};

static const struct pci_device_id nvme_id_table[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_APPLE, 0x2001) },
	{ PCI_VDEVICE(INTEL, 0x5845),   /* Qemu emulated controller */
		.driver_data = NVME_QUIRK_IDENTIFY_CNS, },
    { 0, }
};
MODULE_DEVICE_TABLE(pci, nvme_id_table);

static struct pci_driver nvme_driver = {
    .name       = "menvme",                                                                                                                                                                                 
    .id_table   = nvme_id_table,
    .probe      = nvme_probe,
    .remove     = nvme_remove,
    .shutdown   = nvme_shutdown,
    //.driver     = {  

    .sriov_configure = pci_sriov_configure_simple,
    .err_handler    = &nvme_err_handler,
};

static int __init menvme_init(void)
{
	pr_info("menvme load\n");
	return pci_register_driver(&nvme_driver);
}

static void __exit menvme_exit(void)
{
	pci_unregister_driver(&nvme_driver);
	pr_info("menvme exit\n");
}

module_init(menvme_init);
module_exit(menvme_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
