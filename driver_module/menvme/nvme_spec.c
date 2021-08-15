#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/io-64-nonatomic-lo-hi.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/blk-mq-pci.h>
#include <linux/once.h>
#include <linux/pci.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>
#include <linux/blk-mq-pci.h>
#include <linux/async.h>

#include "menvme.h"

#define SQ_SIZE(depth)		(depth * sizeof(struct nvme_command))
#define CQ_SIZE(depth)		(depth * sizeof(struct nvme_completion))

#define SGES_PER_PAGE	(PAGE_SIZE / sizeof(struct nvme_sgl_desc))

#define NVME_MAX_KB_SZ	4096
#define NVME_MAX_SEGS	127

static unsigned long db_bar_size(struct nvme_dev *dev, unsigned nr_io_queues)
{
	return NVME_REG_DBS + ((nr_io_queues + 1) * 8 * dev->db_stride);
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
	BUG_ON(1);
}

static void nvme_pci_free_ctrl(struct nvme_ctrl *ctrl)
{
#if 1
	struct nvme_dev *dev = to_nvme_dev(ctrl);

	printk("zz %s %d \n", __func__, __LINE__);
	//nvme_dbbuf_dma_free(dev);
	if (dev->tagset.tags)
		blk_mq_free_tag_set(&dev->tagset);
	if (dev->ctrl.admin_q)
		blk_put_queue(dev->ctrl.admin_q);
	//free_opal_dev(dev->ctrl.opal_dev);
	//mempool_destroy(dev->iod_mempool);
#else
	BUG_ON(1);
#endif
}

static void **nvme_pci_iod_list(struct request *req)
{
	struct nvme_iod *iod = blk_mq_rq_to_pdu(req);
	return (void **)(iod->sg + blk_rq_nr_phys_segments(req));
}

static inline unsigned int sq_idx(unsigned int qid, u32 stride)
{
	return qid * 2 * stride;
}

static inline unsigned int cq_idx(unsigned int qid, u32 stride)
{
	return (qid * 2 + 1) * stride;
}

static int nvme_pci_reg_read32(struct nvme_ctrl *ctrl, u32 off, u32 *val)
{
	*val = readl(to_nvme_dev(ctrl)->bar + off);
	return 0;
}

static int nvme_pci_reg_write32(struct nvme_ctrl *ctrl, u32 off, u32 val)
{
	writel(val, to_nvme_dev(ctrl)->bar + off);
	return 0;
}

static int nvme_pci_reg_read64(struct nvme_ctrl *ctrl, u32 off, u64 *val)
{
	*val = readq(to_nvme_dev(ctrl)->bar + off);
	return 0;
}

static int nvme_pci_get_address(struct nvme_ctrl *ctrl, char *buf, int size)
{
	struct pci_dev *pdev = to_pci_dev(to_nvme_dev(ctrl)->dev);

	return snprintf(buf, size, "%s", dev_name(&pdev->dev));
}

static inline bool nvme_cqe_pending(struct nvme_queue *nvmeq)
{
	return (le16_to_cpu(nvmeq->cqes[nvmeq->cq_head].status) & 1) ==
			nvmeq->cq_phase;
}

static inline int nvme_dbbuf_need_event(u16 event_idx, u16 new_idx, u16 old)
{
	return (u16)(new_idx - event_idx - 1) < (u16)(new_idx - old);
}

/* Update dbbuf and return true if an MMIO is required */
static bool nvme_dbbuf_update_and_check_event(u16 value, u32 *dbbuf_db,
					      volatile u32 *dbbuf_ei)
{
	if (dbbuf_db) {
		u16 old_value;

		/*
		 * Ensure that the queue is written before updating
		 * the doorbell in memory
		 */
		wmb();

		old_value = *dbbuf_db;
		*dbbuf_db = value;

		/*
		 * Ensure that the doorbell is updated before reading the event
		 * index from memory.  The controller needs to provide similar
		 * ordering to ensure the envent index is updated before reading
		 * the doorbell.
		 */
		mb();

		if (!nvme_dbbuf_need_event(*dbbuf_ei, value, old_value))
			return false;
	}

	return true;
}

static inline void nvme_ring_cq_doorbell(struct nvme_queue *nvmeq)
{
	u16 head = nvmeq->cq_head;

	if (nvme_dbbuf_update_and_check_event(head, nvmeq->dbbuf_cq_db,
					      nvmeq->dbbuf_cq_ei))
		writel(head, nvmeq->q_db + nvmeq->dev->db_stride);
}

static inline void nvme_handle_cqe(struct nvme_queue *nvmeq, u16 idx)
{
	volatile struct nvme_completion *cqe = &nvmeq->cqes[idx];
	struct request *req;

	if (unlikely(cqe->command_id >= nvmeq->q_depth)) {
		dev_warn(nvmeq->dev->ctrl.device,
			"invalid id %d completed on queue %d\n",
			cqe->command_id, le16_to_cpu(cqe->sq_id));
		return;
	}

	/*
	 * AEN requests are special as they don't time out and can
	 * survive any kind of queue freeze and often don't respond to
	 * aborts.  We don't even bother to allocate a struct request
	 * for them but rather special case them here.
	 */
	if (unlikely(nvmeq->qid == 0 &&
			cqe->command_id >= NVME_AQ_BLK_MQ_DEPTH)) {
		nvme_complete_async_event(&nvmeq->dev->ctrl,
				cqe->status, &cqe->result);
		return;
	}

	req = blk_mq_tag_to_rq(*nvmeq->tags, cqe->command_id);
	nvme_end_request(req, cqe->status, cqe->result);
}

/*
 * Will slightly overestimate the number of pages needed.  This is OK
 * as it only leads to a small amount of wasted memory for the lifetime of
 * the I/O.
 */
static int nvme_npages(unsigned size, struct nvme_dev *dev)
{
	unsigned nprps = DIV_ROUND_UP(size + dev->ctrl.page_size,
				      dev->ctrl.page_size);
	return DIV_ROUND_UP(8 * nprps, PAGE_SIZE - 8);
}

/*
 * Calculates the number of pages needed for the SGL segments. For example a 4k
 * page can accommodate 256 SGL descriptors.
 */
static int nvme_pci_npages_sgl(unsigned int num_seg)
{
	return DIV_ROUND_UP(num_seg * sizeof(struct nvme_sgl_desc), PAGE_SIZE);
}

static unsigned int nvme_pci_iod_alloc_size(struct nvme_dev *dev,
		unsigned int size, unsigned int nseg, bool use_sgl)
{
	size_t alloc_size;

	if (use_sgl)
		alloc_size = sizeof(__le64 *) * nvme_pci_npages_sgl(nseg);
	else
		alloc_size = sizeof(__le64 *) * nvme_npages(size, dev);

	return alloc_size + sizeof(struct scatterlist) * nseg;
}

static unsigned int nvme_pci_cmd_size(struct nvme_dev *dev, bool use_sgl)
{
	unsigned int alloc_size = nvme_pci_iod_alloc_size(dev,
				    NVME_INT_BYTES(dev), NVME_INT_PAGES,
				    use_sgl);

	return sizeof(struct nvme_iod) + alloc_size;
}

static void nvme_complete_cqes(struct nvme_queue *nvmeq, u16 start, u16 end)
{
	while (start != end) {
		nvme_handle_cqe(nvmeq, start);
		if (++start == nvmeq->q_depth)
			start = 0;
	}
}

static inline void nvme_update_cq_head(struct nvme_queue *nvmeq)
{
	if (nvmeq->cq_head == nvmeq->q_depth - 1) {
		nvmeq->cq_head = 0;
		nvmeq->cq_phase = !nvmeq->cq_phase;
	} else {
		nvmeq->cq_head++;
	}
}

static inline int nvme_process_cq(struct nvme_queue *nvmeq, u16 *start,
				  u16 *end, unsigned int tag)
{
	int found = 0;

	*start = nvmeq->cq_head;
	while (nvme_cqe_pending(nvmeq)) {
		if (tag == -1U || nvmeq->cqes[nvmeq->cq_head].command_id == tag)
			found++;
		nvme_update_cq_head(nvmeq);
	}
	*end = nvmeq->cq_head;

	if (*start != *end)
		nvme_ring_cq_doorbell(nvmeq);
	return found;
}

irqreturn_t nvme_irq(int irq, void *data)
{
	struct nvme_queue *nvmeq = data;
	irqreturn_t ret = IRQ_NONE;
	u16 start, end;

	printk("zz %s %d \n", __func__, __LINE__);
	spin_lock(&nvmeq->cq_lock);
	if (nvmeq->cq_head != nvmeq->last_cq_head)
		ret = IRQ_HANDLED;
	nvme_process_cq(nvmeq, &start, &end, -1);
	nvmeq->last_cq_head = nvmeq->cq_head;
	spin_unlock(&nvmeq->cq_lock);

	if (start != end) {
		nvme_complete_cqes(nvmeq, start, end);
		return IRQ_HANDLED;
	}

	return IRQ_HANDLED;
	//return ret;
}

irqreturn_t nvme_irq_check(int irq, void *data)
{
	struct nvme_queue *nvmeq = data;
	if (nvme_cqe_pending(nvmeq))
		return IRQ_WAKE_THREAD;
	return IRQ_NONE;
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


static int nvme_alloc_sq_cmds(struct nvme_dev *dev, struct nvme_queue *nvmeq,
				int qid, int depth)
{
	/* CMB SQEs will be mapped before creation */
	if (qid && dev->cmb && use_cmb_sqes && (dev->cmbsz & NVME_CMBSZ_SQS))
		return 0;

	nvmeq->sq_cmds = dma_alloc_coherent(dev->dev, SQ_SIZE(depth),
					    &nvmeq->sq_dma_addr, GFP_KERNEL);
	if (!nvmeq->sq_cmds)
		return -ENOMEM;
	return 0;

}

static int nvme_alloc_queue(struct nvme_dev *dev, int qid, int depth)
{
	struct nvme_queue *nvmeq = &dev->queues[qid];

	if (dev->ctrl.queue_count > qid)
		return 0;

	nvmeq->cqes = dma_zalloc_coherent(dev->dev, CQ_SIZE(depth),
					  &nvmeq->cq_dma_addr, GFP_KERNEL);
	if (!nvmeq->cqes)
		goto free_nvmeq;

	if (nvme_alloc_sq_cmds(dev, nvmeq, qid, depth))
		goto free_cqdma;

	nvmeq->q_dmadev = dev->dev;
	nvmeq->dev = dev;
	spin_lock_init(&nvmeq->sq_lock);
	spin_lock_init(&nvmeq->cq_lock);
	nvmeq->cq_head = 0;
	nvmeq->cq_phase = 1;
	nvmeq->q_db = &dev->dbs[qid * 2 * dev->db_stride];
	nvmeq->q_depth = depth;
	nvmeq->qid = qid;
	nvmeq->cq_vector = -1;
	dev->ctrl.queue_count++;

	return 0;

 free_cqdma:
	dma_free_coherent(dev->dev, CQ_SIZE(depth), (void *)nvmeq->cqes,
							nvmeq->cq_dma_addr);
 free_nvmeq:
	return -ENOMEM;
}
static int nvme_remap_bar(struct nvme_dev *dev, unsigned long size)
{
	struct pci_dev *pdev = to_pci_dev(dev->dev);

	if (size <= dev->bar_mapped_size)
		return 0;
	if (size > pci_resource_len(pdev, 0))
		return -ENOMEM;
	if (dev->bar)
		iounmap(dev->bar);
	dev->bar = ioremap(pci_resource_start(pdev, 0), size);
	if (!dev->bar) {
		dev->bar_mapped_size = 0;
		return -ENOMEM;
	}
	dev->bar_mapped_size = size;
	dev->dbs = dev->bar + NVME_REG_DBS;

	return 0;
}

static void nvme_free_iod(struct nvme_dev *dev, struct request *req)
{
	struct nvme_iod *iod = blk_mq_rq_to_pdu(req);
	const int last_prp = dev->ctrl.page_size / sizeof(__le64) - 1;
	dma_addr_t dma_addr = iod->first_dma, next_dma_addr;

	int i;

	if (iod->npages == 0)
		dma_pool_free(dev->prp_small_pool, nvme_pci_iod_list(req)[0],
			dma_addr);

	for (i = 0; i < iod->npages; i++) {
		void *addr = nvme_pci_iod_list(req)[i];

		if (iod->use_sgl) {
			struct nvme_sgl_desc *sg_list = addr;

			next_dma_addr =
			    le64_to_cpu((sg_list[SGES_PER_PAGE - 1]).addr);
		} else {
			__le64 *prp_list = addr;

			next_dma_addr = le64_to_cpu(prp_list[last_prp]);
		}

		dma_pool_free(dev->prp_page_pool, addr, dma_addr);
		dma_addr = next_dma_addr;
	}

	if (iod->sg != iod->inline_sg)
		mempool_free(iod->sg, dev->iod_mempool);
}


/**
 * nvme_submit_cmd() - Copy a command into a queue and ring the doorbell
 * @nvmeq: The queue to use
 * @cmd: The command to send
 */
static void nvme_submit_cmd(struct nvme_queue *nvmeq, struct nvme_command *cmd)
{
	printk("zz %s %d \n", __func__, __LINE__);
	spin_lock(&nvmeq->sq_lock);
	if (nvmeq->sq_cmds_io)
		memcpy_toio(&nvmeq->sq_cmds_io[nvmeq->sq_tail], cmd,
				sizeof(*cmd));
	else
		memcpy(&nvmeq->sq_cmds[nvmeq->sq_tail], cmd, sizeof(*cmd));

	if (++nvmeq->sq_tail == nvmeq->q_depth)
		nvmeq->sq_tail = 0;
	if (nvme_dbbuf_update_and_check_event(nvmeq->sq_tail,
			nvmeq->dbbuf_sq_db, nvmeq->dbbuf_sq_ei))
		writel(nvmeq->sq_tail, nvmeq->q_db);
	spin_unlock(&nvmeq->sq_lock);
}

static void nvme_dbbuf_init(struct nvme_dev *dev,
			    struct nvme_queue *nvmeq, int qid)
{
	if (!dev->dbbuf_dbs || !qid)
		return;

	nvmeq->dbbuf_sq_db = &dev->dbbuf_dbs[sq_idx(qid, dev->db_stride)];
	nvmeq->dbbuf_cq_db = &dev->dbbuf_dbs[cq_idx(qid, dev->db_stride)];
	nvmeq->dbbuf_sq_ei = &dev->dbbuf_eis[sq_idx(qid, dev->db_stride)];
	nvmeq->dbbuf_cq_ei = &dev->dbbuf_eis[cq_idx(qid, dev->db_stride)];
}

static void nvme_init_queue(struct nvme_queue *nvmeq, u16 qid)
{
	struct nvme_dev *dev = nvmeq->dev;

	spin_lock_irq(&nvmeq->cq_lock);
	nvmeq->sq_tail = 0;
	nvmeq->cq_head = 0;
	nvmeq->cq_phase = 1;
	nvmeq->q_db = &dev->dbs[qid * 2 * dev->db_stride];
	memset((void *)nvmeq->cqes, 0, CQ_SIZE(nvmeq->q_depth));
	nvme_dbbuf_init(dev, nvmeq, qid);
	dev->online_queues++;
	spin_unlock_irq(&nvmeq->cq_lock);
}


static int nvme_pci_configure_admin_queue(struct nvme_dev *dev)
{
	int result;
	u32 aqa;
	struct nvme_queue *nvmeq;

	result = nvme_remap_bar(dev, db_bar_size(dev, 0));
	if (result < 0)
		return result;

	dev->subsystem = readl(dev->bar + NVME_REG_VS) >= NVME_VS(1, 1, 0) ?
				NVME_CAP_NSSRC(dev->ctrl.cap) : 0;

	if (dev->subsystem &&
	    (readl(dev->bar + NVME_REG_CSTS) & NVME_CSTS_NSSRO))
		writel(NVME_CSTS_NSSRO, dev->bar + NVME_REG_CSTS);

	result = nvme_disable_ctrl(&dev->ctrl, dev->ctrl.cap);
	if (result < 0)
		return result;

	result = nvme_alloc_queue(dev, 0, NVME_AQ_DEPTH);
	if (result)
		return result;

	nvmeq = &dev->queues[0];
	aqa = nvmeq->q_depth - 1;
	aqa |= aqa << 16;

	writel(aqa, dev->bar + NVME_REG_AQA);
	lo_hi_writeq(nvmeq->sq_dma_addr, dev->bar + NVME_REG_ASQ);
	lo_hi_writeq(nvmeq->cq_dma_addr, dev->bar + NVME_REG_ACQ);

	result = nvme_enable_ctrl(&dev->ctrl, dev->ctrl.cap);
	if (result)
		return result;

	nvmeq->cq_vector = 0;
	nvme_init_queue(nvmeq, 0);
	result = nvme_pci_queue_request_irq(nvmeq);
	if (result) {
		nvmeq->cq_vector = -1;
		return result;
	}

	pr_info("bar:%lx size:%lx \n", dev->bar, dev->bar_mapped_size);
	pr_info("cqes:%lx cq_dma_addr:%lx \n", nvmeq->cqes, CQ_SIZE(NVME_AQ_DEPTH));
	pr_info("sq_cmds:%lx sq_dma_addr:%lx",nvmeq->sq_cmds, CQ_SIZE(NVME_AQ_DEPTH));

	return result;
}

static inline bool nvme_pci_use_sgls(struct nvme_dev *dev, struct request *req)
{
	struct nvme_iod *iod = blk_mq_rq_to_pdu(req);
	int nseg = blk_rq_nr_phys_segments(req);
	unsigned int avg_seg_size;

	if (nseg == 0)
		return false;

	avg_seg_size = DIV_ROUND_UP(blk_rq_payload_bytes(req), nseg);

	if (!(dev->ctrl.sgls & ((1 << 0) | (1 << 1))))
		return false;
	if (!iod->nvmeq->qid)
		return false;
	if (!sgl_threshold || avg_seg_size < sgl_threshold)
		return false;
	return true;
}

static blk_status_t nvme_init_iod(struct request *rq, struct nvme_dev *dev)
{
	struct nvme_iod *iod = blk_mq_rq_to_pdu(rq);
	int nseg = blk_rq_nr_phys_segments(rq);
	unsigned int size = blk_rq_payload_bytes(rq);

	iod->use_sgl = nvme_pci_use_sgls(dev, rq);

	if (nseg > NVME_INT_PAGES || size > NVME_INT_BYTES(dev)) {
		iod->sg = mempool_alloc(dev->iod_mempool, GFP_ATOMIC);
		if (!iod->sg)
			return BLK_STS_RESOURCE;
	} else {
		iod->sg = iod->inline_sg;
	}

	iod->aborted = 0;
	iod->npages = -1;
	iod->nents = 0;
	iod->length = size;

	return BLK_STS_OK;
}

static void nvme_print_sgl(struct scatterlist *sgl, int nents)
{
	int i;
	struct scatterlist *sg;

	for_each_sg(sgl, sg, nents, i) {
		dma_addr_t phys = sg_phys(sg);
		pr_warn("sg[%d] phys_addr:%pad offset:%d length:%d "
			"dma_address:%pad dma_length:%d\n",
			i, &phys, sg->offset, sg->length, &sg_dma_address(sg),
			sg_dma_len(sg));
	}
}

static blk_status_t nvme_pci_setup_prps(struct nvme_dev *dev,
		struct request *req, struct nvme_rw_command *cmnd)
{
	struct nvme_iod *iod = blk_mq_rq_to_pdu(req);
	struct dma_pool *pool;
	int length = blk_rq_payload_bytes(req);
	struct scatterlist *sg = iod->sg;
	int dma_len = sg_dma_len(sg);
	u64 dma_addr = sg_dma_address(sg);
	u32 page_size = dev->ctrl.page_size;
	int offset = dma_addr & (page_size - 1);
	__le64 *prp_list;
	void **list = nvme_pci_iod_list(req);
	dma_addr_t prp_dma;
	int nprps, i;

	length -= (page_size - offset);
	if (length <= 0) {
		iod->first_dma = 0;
		goto done;
	}

	dma_len -= (page_size - offset);
	if (dma_len) {
		dma_addr += (page_size - offset);
	} else {
		sg = sg_next(sg);
		dma_addr = sg_dma_address(sg);
		dma_len = sg_dma_len(sg);
	}

	if (length <= page_size) {
		iod->first_dma = dma_addr;
		goto done;
	}

	nprps = DIV_ROUND_UP(length, page_size);
	if (nprps <= (256 / 8)) {
		pool = dev->prp_small_pool;
		iod->npages = 0;
	} else {
		pool = dev->prp_page_pool;
		iod->npages = 1;
	}

	prp_list = dma_pool_alloc(pool, GFP_ATOMIC, &prp_dma);
	if (!prp_list) {
		iod->first_dma = dma_addr;
		iod->npages = -1;
		return BLK_STS_RESOURCE;
	}
	list[0] = prp_list;
	iod->first_dma = prp_dma;
	i = 0;
	for (;;) {
		if (i == page_size >> 3) {
			__le64 *old_prp_list = prp_list;
			prp_list = dma_pool_alloc(pool, GFP_ATOMIC, &prp_dma);
			if (!prp_list)
				return BLK_STS_RESOURCE;
			list[iod->npages++] = prp_list;
			prp_list[0] = old_prp_list[i - 1];
			old_prp_list[i - 1] = cpu_to_le64(prp_dma);
			i = 1;
		}
		prp_list[i++] = cpu_to_le64(dma_addr);
		dma_len -= page_size;
		dma_addr += page_size;
		length -= page_size;
		if (length <= 0)
			break;
		if (dma_len > 0)
			continue;
		if (unlikely(dma_len < 0))
			goto bad_sgl;
		sg = sg_next(sg);
		dma_addr = sg_dma_address(sg);
		dma_len = sg_dma_len(sg);
	}

done:
	cmnd->dptr.prp1 = cpu_to_le64(sg_dma_address(iod->sg));
	cmnd->dptr.prp2 = cpu_to_le64(iod->first_dma);

	return BLK_STS_OK;

 bad_sgl:
	WARN(DO_ONCE(nvme_print_sgl, iod->sg, iod->nents),
			"Invalid SGL for payload:%d nents:%d\n",
			blk_rq_payload_bytes(req), iod->nents);
	return BLK_STS_IOERR;
}

static void nvme_pci_sgl_set_data(struct nvme_sgl_desc *sge,
		struct scatterlist *sg)
{
	sge->addr = cpu_to_le64(sg_dma_address(sg));
	sge->length = cpu_to_le32(sg_dma_len(sg));
	sge->type = NVME_SGL_FMT_DATA_DESC << 4;
}

static void nvme_pci_sgl_set_seg(struct nvme_sgl_desc *sge,
		dma_addr_t dma_addr, int entries)
{
	sge->addr = cpu_to_le64(dma_addr);
	if (entries < SGES_PER_PAGE) {
		sge->length = cpu_to_le32(entries * sizeof(*sge));
		sge->type = NVME_SGL_FMT_LAST_SEG_DESC << 4;
	} else {
		sge->length = cpu_to_le32(PAGE_SIZE);
		sge->type = NVME_SGL_FMT_SEG_DESC << 4;
	}
}

static blk_status_t nvme_pci_setup_sgls(struct nvme_dev *dev,
		struct request *req, struct nvme_rw_command *cmd, int entries)
{
	struct nvme_iod *iod = blk_mq_rq_to_pdu(req);
	struct dma_pool *pool;
	struct nvme_sgl_desc *sg_list;
	struct scatterlist *sg = iod->sg;
	dma_addr_t sgl_dma;
	int i = 0;

	/* setting the transfer type as SGL */
	cmd->flags = NVME_CMD_SGL_METABUF;

	if (entries == 1) {
		nvme_pci_sgl_set_data(&cmd->dptr.sgl, sg);
		return BLK_STS_OK;
	}

	if (entries <= (256 / sizeof(struct nvme_sgl_desc))) {
		pool = dev->prp_small_pool;
		iod->npages = 0;
	} else {
		pool = dev->prp_page_pool;
		iod->npages = 1;
	}

	sg_list = dma_pool_alloc(pool, GFP_ATOMIC, &sgl_dma);
	if (!sg_list) {
		iod->npages = -1;
		return BLK_STS_RESOURCE;
	}

	nvme_pci_iod_list(req)[0] = sg_list;
	iod->first_dma = sgl_dma;

	nvme_pci_sgl_set_seg(&cmd->dptr.sgl, sgl_dma, entries);

	do {
		if (i == SGES_PER_PAGE) {
			struct nvme_sgl_desc *old_sg_desc = sg_list;
			struct nvme_sgl_desc *link = &old_sg_desc[i - 1];

			sg_list = dma_pool_alloc(pool, GFP_ATOMIC, &sgl_dma);
			if (!sg_list)
				return BLK_STS_RESOURCE;

			i = 0;
			nvme_pci_iod_list(req)[iod->npages++] = sg_list;
			sg_list[i++] = *link;
			nvme_pci_sgl_set_seg(link, sgl_dma, entries);
		}

		nvme_pci_sgl_set_data(&sg_list[i++], sg);
		sg = sg_next(sg);
	} while (--entries > 0);

	return BLK_STS_OK;
}
static blk_status_t nvme_map_data(struct nvme_dev *dev, struct request *req,
		struct nvme_command *cmnd)
{
	struct nvme_iod *iod = blk_mq_rq_to_pdu(req);
	struct request_queue *q = req->q;
	enum dma_data_direction dma_dir = rq_data_dir(req) ?
			DMA_TO_DEVICE : DMA_FROM_DEVICE;
	blk_status_t ret = BLK_STS_IOERR;
	int nr_mapped;

	sg_init_table(iod->sg, blk_rq_nr_phys_segments(req));
	iod->nents = blk_rq_map_sg(q, req, iod->sg);
	if (!iod->nents)
		goto out;

	ret = BLK_STS_RESOURCE;
	nr_mapped = dma_map_sg_attrs(dev->dev, iod->sg, iod->nents, dma_dir,
			DMA_ATTR_NO_WARN);
	if (!nr_mapped)
		goto out;

	if (iod->use_sgl)
		ret = nvme_pci_setup_sgls(dev, req, &cmnd->rw, nr_mapped);
	else
		ret = nvme_pci_setup_prps(dev, req, &cmnd->rw);

	if (ret != BLK_STS_OK)
		goto out_unmap;

	ret = BLK_STS_IOERR;
	if (blk_integrity_rq(req)) {
		if (blk_rq_count_integrity_sg(q, req->bio) != 1)
			goto out_unmap;

		sg_init_table(&iod->meta_sg, 1);
		if (blk_rq_map_integrity_sg(q, req->bio, &iod->meta_sg) != 1)
			goto out_unmap;

		if (!dma_map_sg(dev->dev, &iod->meta_sg, 1, dma_dir))
			goto out_unmap;
	}

	if (blk_integrity_rq(req))
		cmnd->rw.metadata = cpu_to_le64(sg_dma_address(&iod->meta_sg));
	return BLK_STS_OK;

out_unmap:
	dma_unmap_sg(dev->dev, iod->sg, iod->nents, dma_dir);
out:
	return ret;
}

static void nvme_unmap_data(struct nvme_dev *dev, struct request *req)
{
	struct nvme_iod *iod = blk_mq_rq_to_pdu(req);
	enum dma_data_direction dma_dir = rq_data_dir(req) ?
			DMA_TO_DEVICE : DMA_FROM_DEVICE;

	if (iod->nents) {
		dma_unmap_sg(dev->dev, iod->sg, iod->nents, dma_dir);
		if (blk_integrity_rq(req))
			dma_unmap_sg(dev->dev, &iod->meta_sg, 1, dma_dir);
	}

	nvme_cleanup_cmd(req);
	nvme_free_iod(dev, req);
}

static void dump_request(struct request*req)
{
	printk("bio:%lx \n", (unsigned long)req->bio);
	printk("biotail:%lx \n", (unsigned long)req->biotail);
	printk("start_time_ns:%lx \n", (unsigned long)req->start_time_ns);
	printk("io_start_time_ns:%lx \n", (unsigned long)req->io_start_time_ns);
	printk("nr_phys_segments:%lx \n", (unsigned long)req->nr_phys_segments);
	printk("end_io:%lx \n", (unsigned long)req->end_io);
	printk("next_rq:%lx \n", (unsigned long)req->next_rq);
}
static void dump_request_queue(struct request_queue *req_q)
{
	printk("request_fn:%lx \n", (unsigned long)req_q->request_fn);
	printk("make_request_fn:%lx \n", (unsigned long)req_q->make_request_fn);
	printk("prep_rq_fn:%lx \n", (unsigned long)req_q->prep_rq_fn);
	printk("unprep_rq_fn:%lx \n", (unsigned long)req_q->unprep_rq_fn);
	printk("softirq_done_fn:%lx \n", (unsigned long)req_q->softirq_done_fn);
	printk("rq_timed_out_fn:%lx \n", (unsigned long)req_q->rq_timed_out_fn);
	printk("dma_drain_needed:%lx \n", (unsigned long)req_q->dma_drain_needed);
	printk("lld_busy_fn:%lx \n", (unsigned long)req_q->lld_busy_fn);
	printk("init_rq_fn:%lx \n", (unsigned long)req_q->init_rq_fn);
	printk("exit_rq_fn:%lx \n", (unsigned long)req_q->exit_rq_fn);
	printk("nr_queues:%lx \n", (unsigned long)req_q->nr_queues);
	printk("queue_depth:%lx \n", (unsigned long)req_q->queue_depth);
	printk("nr_requests:%lx \n", (unsigned long)req_q->nr_requests);
	printk("nr_congestion_on:%lx \n", (unsigned long)req_q->nr_congestion_on);
	printk("nr_congestion_off:%lx \n", (unsigned long)req_q->nr_congestion_off);
	printk("nr_batching:%lx \n", (unsigned long)req_q->nr_batching);
}

/*
 * NOTE: ns is NULL when called on the admin queue.
 */
static blk_status_t nvme_queue_rq(struct blk_mq_hw_ctx *hctx,
			 const struct blk_mq_queue_data *bd)
{
	struct nvme_ns *ns = hctx->queue->queuedata;
	struct nvme_queue *nvmeq = hctx->driver_data;
	struct nvme_dev *dev = nvmeq->dev;
	struct request *req = bd->rq;
	struct nvme_command cmnd;
	blk_status_t ret;
	printk("zz %s %d \n", __func__, __LINE__);
	dump_request_queue(dev->ctrl.admin_q);
	dump_request(req);
	dump_stack();


	/*
	 * We should not need to do this, but we're still using this to
	 * ensure we can drain requests on a dying queue.
	 */
	if (unlikely(nvmeq->cq_vector < 0))
		return BLK_STS_IOERR;

	ret = nvme_setup_cmd(ns, req, &cmnd);
	if (ret)
		return ret;

	ret = nvme_init_iod(req, dev);
	if (ret)
		goto out_free_cmd;

	if (blk_rq_nr_phys_segments(req)) {
		ret = nvme_map_data(dev, req, &cmnd);
		if (ret)
			goto out_cleanup_iod;
	}

	blk_mq_start_request(req);
	nvme_submit_cmd(nvmeq, &cmnd);
	return BLK_STS_OK;
out_cleanup_iod:
	nvme_free_iod(dev, req);
out_free_cmd:
	nvme_cleanup_cmd(req);
	return ret;
}

static void nvme_pci_complete_rq(struct request *req)
{
	struct nvme_iod *iod = blk_mq_rq_to_pdu(req);

	nvme_unmap_data(iod->nvmeq->dev, req);
	nvme_complete_rq(req);
}

static int nvme_admin_init_hctx(struct blk_mq_hw_ctx *hctx, void *data,
				unsigned int hctx_idx)
{
	struct nvme_dev *dev = data;
	struct nvme_queue *nvmeq = &dev->queues[0];

	WARN_ON(hctx_idx != 0);
	WARN_ON(dev->admin_tagset.tags[0] != hctx->tags);
	WARN_ON(nvmeq->tags);

	hctx->driver_data = nvmeq;
	nvmeq->tags = &dev->admin_tagset.tags[0];
	return 0;
}

static void nvme_admin_exit_hctx(struct blk_mq_hw_ctx *hctx, unsigned int hctx_idx)
{
	struct nvme_queue *nvmeq = hctx->driver_data;

	nvmeq->tags = NULL;
}

static int nvme_init_hctx(struct blk_mq_hw_ctx *hctx, void *data,
			  unsigned int hctx_idx)
{
	struct nvme_dev *dev = data;
	struct nvme_queue *nvmeq = &dev->queues[hctx_idx + 1];

	if (!nvmeq->tags)
		nvmeq->tags = &dev->tagset.tags[hctx_idx];

	WARN_ON(dev->tagset.tags[hctx_idx] != hctx->tags);
	hctx->driver_data = nvmeq;
	return 0;
}

static int nvme_init_request(struct blk_mq_tag_set *set, struct request *req,
		unsigned int hctx_idx, unsigned int numa_node)
{
	struct nvme_dev *dev = set->driver_data;
	struct nvme_iod *iod = blk_mq_rq_to_pdu(req);
	int queue_idx = (set == &dev->tagset) ? hctx_idx + 1 : 0;
	struct nvme_queue *nvmeq = &dev->queues[queue_idx];

	BUG_ON(!nvmeq);
	iod->nvmeq = nvmeq;

	nvme_req(req)->ctrl = &dev->ctrl;
	return 0;
}

static enum blk_eh_timer_return nvme_timeout(struct request *req, bool reserved)
{
	return BLK_EH_RESET_TIMER;
}

static int __nvme_poll(struct nvme_queue *nvmeq, unsigned int tag)
{
	u16 start, end;
	int found;

	if (!nvme_cqe_pending(nvmeq))
		return 0;

	spin_lock_irq(&nvmeq->cq_lock);
	found = nvme_process_cq(nvmeq, &start, &end, tag);
	spin_unlock_irq(&nvmeq->cq_lock);

	nvme_complete_cqes(nvmeq, start, end);
	return found;
}

static int nvme_poll(struct blk_mq_hw_ctx *hctx, unsigned int tag)
{
	struct nvme_queue *nvmeq = hctx->driver_data;

	return __nvme_poll(nvmeq, tag);
}

static int nvme_pci_map_queues(struct blk_mq_tag_set *set)
{
	struct nvme_dev *dev = set->driver_data;

	return blk_mq_pci_map_queues(set, to_pci_dev(dev->dev),
			dev->num_vecs > 1 ? 1 /* admin queue */ : 0);
}

static const struct blk_mq_ops nvme_mq_admin_ops = {
	.queue_rq	= nvme_queue_rq,
	.complete	= nvme_pci_complete_rq,
	.init_hctx	= nvme_admin_init_hctx,
	.exit_hctx      = nvme_admin_exit_hctx,
	.init_request	= nvme_init_request,
	.timeout	= nvme_timeout,
};

static const struct blk_mq_ops nvme_mq_ops = {
	.queue_rq	= nvme_queue_rq,
	.complete	= nvme_pci_complete_rq,
	.init_hctx	= nvme_init_hctx,
	.init_request	= nvme_init_request,
	.map_queues	= nvme_pci_map_queues,
	.timeout	= nvme_timeout,
	.poll		= nvme_poll,
};

static void nvme_dev_remove_admin(struct nvme_dev *dev)
{
	if (dev->ctrl.admin_q && !blk_queue_dying(dev->ctrl.admin_q)) {
		/*
		 * If the controller was reset during removal, it's possible
		 * user requests may be waiting on a stopped queue. Start the
		 * queue to flush these to completion.
		 */
		blk_mq_unquiesce_queue(dev->ctrl.admin_q);
		blk_cleanup_queue(dev->ctrl.admin_q);
		blk_mq_free_tag_set(&dev->admin_tagset);
	}
}

static int nvme_alloc_admin_tags(struct nvme_dev *dev)
{
	if (!dev->ctrl.admin_q) {
		dev->admin_tagset.ops = &nvme_mq_admin_ops;
		dev->admin_tagset.nr_hw_queues = 1;

		dev->admin_tagset.queue_depth = NVME_AQ_MQ_TAG_DEPTH;
		dev->admin_tagset.timeout = ADMIN_TIMEOUT;
		dev->admin_tagset.numa_node = dev_to_node(dev->dev);
		dev->admin_tagset.cmd_size = nvme_pci_cmd_size(dev, false);
		dev->admin_tagset.flags = BLK_MQ_F_NO_SCHED;
		dev->admin_tagset.driver_data = dev;

		if (blk_mq_alloc_tag_set(&dev->admin_tagset))
			return -ENOMEM;
		dev->ctrl.admin_tagset = &dev->admin_tagset;

		dev->ctrl.admin_q = blk_mq_init_queue(&dev->admin_tagset);
		if (IS_ERR(dev->ctrl.admin_q)) {
			blk_mq_free_tag_set(&dev->admin_tagset);
			return -ENOMEM;
		}
		if (!blk_get_queue(dev->ctrl.admin_q)) {
			nvme_dev_remove_admin(dev);
			dev->ctrl.admin_q = NULL;
			return -ENODEV;
		}
	} else
		blk_mq_unquiesce_queue(dev->ctrl.admin_q);

	return 0;
}


static void nvme_reset_work(struct work_struct *work)
{
	struct nvme_dev *dev =
		container_of(work, struct nvme_dev, ctrl.reset_work);
	bool was_suspend = !!(dev->ctrl.ctrl_config & NVME_CC_SHN_NORMAL);
	int result;

	if (WARN_ON(dev->ctrl.state != NVME_CTRL_RESETTING)) {
		result = -ENODEV;
		goto out;
	}

	/*
	 * If we're called to reset a live controller first shut it down before
	 * moving on.
	 */
	//if (dev->ctrl.ctrl_config & NVME_CC_ENABLE)
	//	nvme_dev_disable(dev, false);

	//not need, it has been enable in probe
	/*result = nvme_pci_enable(dev);
	if (result)
		goto out_unlock;
	*/

	result = nvme_pci_configure_admin_queue(dev);
	if (result)
		goto out_unlock;

	result = nvme_alloc_admin_tags(dev);
	if (result)
		goto out_unlock;

	dev->ctrl.max_hw_sectors = NVME_MAX_KB_SZ << 1;
	dev->ctrl.max_segments = NVME_MAX_SEGS;
	if (!nvme_change_ctrl_state(&dev->ctrl, NVME_CTRL_CONNECTING)) {
		dev_warn(dev->ctrl.device,
			"failed to mark controller CONNECTING\n");
		result = -EBUSY;
		goto out;
	}

	result = nvme_init_identify(&dev->ctrl);
	if (result)
		goto out;

out_unlock:
out:
	printk("zz %s %d \n", __func__, __LINE__);
}

static void nvme_async_probe(void *data, async_cookie_t cookie)
{
	struct nvme_dev *dev = data;

	flush_work(&dev->ctrl.reset_work);
	flush_work(&dev->ctrl.scan_work);
	nvme_put_ctrl(&dev->ctrl);
}

int menvme_init_ctrl(struct nvme_dev *dev, unsigned long quirks )
{
	int result = -ENOMEM;
	result = nvme_init_ctrl(&dev->ctrl, dev->dev, &nvme_pci_ctrl_ops,
			quirks);

	INIT_WORK(&dev->ctrl.reset_work, nvme_reset_work);
	nvme_reset_ctrl(&dev->ctrl);
	nvme_get_ctrl(&dev->ctrl);
	async_schedule(nvme_async_probe, dev);

}

void menvme_free_ctrl(struct nvme_dev *dev)
{

	flush_work(&dev->ctrl.reset_work);
	nvme_stop_ctrl(&dev->ctrl);
#if 0
	nvme_remove_namespaces(&dev->ctrl);
	nvme_dev_disable(dev, true);
#endif
	//nvme_release_cmb(dev);
#if 0
	nvme_free_host_mem(dev);
	nvme_dev_remove_admin(dev);
	nvme_free_queues(dev, 0);
#endif
	nvme_uninit_ctrl(&dev->ctrl);
#if 0
	nvme_release_prp_pools(dev);
#endif
	//nvme_dev_unmap(dev);
#if 0
	nvme_put_ctrl(&dev->ctrl);
#endif
	nvme_put_ctrl(&dev->ctrl);
}
