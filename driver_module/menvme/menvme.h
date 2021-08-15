#ifndef  __MENVME_H__
#define  __MENVME_H__
#include <linux/nvme.h>
#include <linux/cdev.h>
#include <linux/pci.h>
#include <linux/kref.h>
#include <linux/blk-mq.h>
#include <linux/lightnvm.h>
#include <linux/sed-opal.h>
#include <linux/fault-inject.h>
#include <linux/rcupdate.h>

#define NVME_INT_PAGES		2
#define NVME_INT_BYTES(dev)	(NVME_INT_PAGES * (dev)->ctrl.page_size)

extern unsigned int admin_timeout;
#define ADMIN_TIMEOUT	(admin_timeout * HZ)

extern bool use_cmb_sqes;
extern int io_queue_depth;
extern unsigned int sgl_threshold;

struct nvme_dev;
struct nvme_queue;
struct nvme_ctrl;

enum nvme_ctrl_state {
	NVME_CTRL_NEW,
	NVME_CTRL_LIVE,
	NVME_CTRL_ADMIN_ONLY,    /* Only admin queue live */
	NVME_CTRL_RESETTING,
	NVME_CTRL_CONNECTING,
	NVME_CTRL_DELETING,
	NVME_CTRL_DEAD,
};


struct nvme_ctrl {
	enum nvme_ctrl_state state;
	bool identified;
	spinlock_t lock;
	struct mutex scan_lock;
	const struct nvme_ctrl_ops *ops;
	struct request_queue *admin_q;
	struct request_queue *connect_q;
	struct device *dev;
	int instance;
	struct blk_mq_tag_set *tagset;
	struct blk_mq_tag_set *admin_tagset;
	struct list_head namespaces;
	struct rw_semaphore namespaces_rwsem;
	struct device ctrl_device;
	struct device *device;	/* char device */
	struct cdev cdev;
	struct work_struct reset_work;
	struct work_struct delete_work;

	struct nvme_subsystem *subsys;
	struct list_head subsys_entry;

	struct opal_dev *opal_dev;

	char name[12];
	u16 cntlid;

	u32 ctrl_config;
	u16 mtfa;
	u32 queue_count;

	u64 cap;
	u32 page_size;
	u32 max_hw_sectors;
	u32 max_segments;
	u16 oncs;
	u16 oacs;
	u16 nssa;
	u16 nr_streams;
	u32 max_namespaces;
	atomic_t abort_limit;
	u8 vwc;
	u32 vs;
	u32 sgls;
	u16 kas;
	u8 npss;
	u8 apsta;
	u32 oaes;
	u32 aen_result;
	unsigned int shutdown_timeout;
	unsigned int kato;
	bool subsystem;
	unsigned long quirks;
	struct nvme_id_power_state psd[32];
	struct nvme_effects_log *effects;
	struct work_struct scan_work;
	struct work_struct async_event_work;
	struct delayed_work ka_work;
	struct nvme_command ka_cmd;
	struct work_struct fw_act_work;
	unsigned long events;

#ifdef CONFIG_NVME_MULTIPATH
	/* asymmetric namespace access: */
	u8 anacap;
	u8 anatt;
	u32 anagrpmax;
	u32 nanagrpid;
	struct mutex ana_lock;
	struct nvme_ana_rsp_hdr *ana_log_buf;
	size_t ana_log_size;
	struct timer_list anatt_timer;
	struct work_struct ana_work;
#endif

	/* Power saving configuration */
	u64 ps_max_latency_us;
	bool apst_enabled;

	/* PCIe only: */
	u32 hmpre;
	u32 hmmin;
	u32 hmminds;
	u16 hmmaxd;

	/* Fabrics only */
	u16 sqsize;
	u32 ioccsz;
	u32 iorcsz;
	u16 icdoff;
	u16 maxcmd;
	int nr_reconnects;
	struct nvmf_ctrl_options *opts;

	struct page *discard_page;
	unsigned long discard_page_busy;
};

struct nvme_dev {
	struct nvme_queue *queues;
	struct blk_mq_tag_set tagset;
	struct blk_mq_tag_set admin_tagset;
	u32 __iomem *dbs;
	struct device *dev;
	struct dma_pool *prp_page_pool;
	struct dma_pool *prp_small_pool;
	unsigned online_queues;
	unsigned max_qid;
	unsigned int num_vecs;
	int q_depth;
	u32 db_stride;
	void __iomem *bar;
	unsigned long bar_mapped_size;
	struct work_struct remove_work;
	struct mutex shutdown_lock;
	bool subsystem;
	void __iomem *cmb;
	pci_bus_addr_t cmb_bus_addr;
	u64 cmb_size;
	u32 cmbsz;
	u32 cmbloc;
	struct nvme_ctrl ctrl;
	struct completion ioq_wait;

	mempool_t *iod_mempool;

	/* shadow doorbell buffer support: */
	u32 *dbbuf_dbs;
	dma_addr_t dbbuf_dbs_dma_addr;
	u32 *dbbuf_eis;
	dma_addr_t dbbuf_eis_dma_addr;

	/* host memory buffer support: */
	u64 host_mem_size;
	u32 nr_host_mem_descs;
	dma_addr_t host_mem_descs_dma;
	//struct nvme_host_mem_buf_desc *host_mem_descs;
	void **host_mem_desc_bufs;
};

struct nvme_queue {
	struct device *q_dmadev;
	struct nvme_dev *dev;
	spinlock_t sq_lock;
	struct nvme_command *sq_cmds;
	struct nvme_command __iomem *sq_cmds_io;
	spinlock_t cq_lock ____cacheline_aligned_in_smp;
	volatile struct nvme_completion *cqes;
	struct blk_mq_tags **tags;
	dma_addr_t sq_dma_addr;
	dma_addr_t cq_dma_addr;
	u32 __iomem *q_db;
	u16 q_depth;
	s16 cq_vector;
	u16 sq_tail;
	u16 cq_head;
	u16 last_cq_head;
	u16 qid;
	u8 cq_phase;
	u32 *dbbuf_sq_db;
	u32 *dbbuf_cq_db;
	u32 *dbbuf_sq_ei;
	u32 *dbbuf_cq_ei;
};


struct nvme_ctrl_ops {
	const char *name;
	struct module *module;
	unsigned int flags;
#define NVME_F_FABRICS			(1 << 0)
#define NVME_F_METADATA_SUPPORTED	(1 << 1)
	int (*reg_read32)(struct nvme_ctrl *ctrl, u32 off, u32 *val);
	int (*reg_write32)(struct nvme_ctrl *ctrl, u32 off, u32 val);
	int (*reg_read64)(struct nvme_ctrl *ctrl, u32 off, u64 *val);
	void (*free_ctrl)(struct nvme_ctrl *ctrl);
	void (*submit_async_event)(struct nvme_ctrl *ctrl);
	void (*delete_ctrl)(struct nvme_ctrl *ctrl);
	int (*get_address)(struct nvme_ctrl *ctrl, char *buf, int size);
	void (*stop_ctrl)(struct nvme_ctrl *ctrl);
};

/*
 * Common request structure for NVMe passthrough.  All drivers must have
 * this structure as the first member of their request-private data.
 */
struct nvme_request {
	struct nvme_command	*cmd;
	union nvme_result	result;
	u8			retries;
	u8			flags;
	u16			status;
	struct nvme_ctrl	*ctrl;
};

struct nvme_ns {
	struct list_head list;

	struct nvme_ctrl *ctrl;
	struct request_queue *queue;
	struct gendisk *disk;
#ifdef CONFIG_NVME_MULTIPATH
	enum nvme_ana_state ana_state;
	u32 ana_grpid;
#endif
	struct list_head siblings;
	struct nvm_dev *ndev;
	struct kref kref;
	struct nvme_ns_head *head;

	int lba_shift;
	u16 ms;
	u16 sgs;
	u32 sws;
	bool ext;
	u8 pi_type;
	unsigned long flags;
#define NVME_NS_REMOVING	0
#define NVME_NS_DEAD     	1
#define NVME_NS_ANA_PENDING	2
	u16 noiob;

#ifdef CONFIG_FAULT_INJECTION_DEBUG_FS
	struct nvme_fault_inject fault_inject;
#endif

};

/*
 * The nvme_iod describes the data in an I/O, including the list of PRP
 * entries.  You can't see it in this data structure because C doesn't let
 * me express that.  Use nvme_init_iod to ensure there's enough space
 * allocated to store the PRP list.
 */
struct nvme_iod {
	struct nvme_request req;
	struct nvme_queue *nvmeq;
	bool use_sgl;
	int aborted;
	int npages;		/* In the PRP list. 0 means small pool in use */
	int nents;		/* Used in scatterlist */
	int length;		/* Of data, in bytes */
	dma_addr_t first_dma;
	struct scatterlist meta_sg; /* metadata requires single contiguous buffer */
	struct scatterlist *sg;
	struct scatterlist inline_sg[0];
};

#ifdef CONFIG_FAULT_INJECTION_DEBUG_FS
void nvme_fault_inject_init(struct nvme_ns *ns);
void nvme_fault_inject_fini(struct nvme_ns *ns);
void nvme_should_fail(struct request *req);
#else
static inline void nvme_fault_inject_init(struct nvme_ns *ns) {}
static inline void nvme_fault_inject_fini(struct nvme_ns *ns) {}
static inline void nvme_should_fail(struct request *req) {}
#endif

static inline struct nvme_request *nvme_req(struct request *req)
{
	return blk_mq_rq_to_pdu(req);
}

static inline struct nvme_dev *to_nvme_dev(struct nvme_ctrl *ctrl)
{
	return container_of(ctrl, struct nvme_dev, ctrl);
}

static inline void nvme_get_ctrl(struct nvme_ctrl *ctrl)
{
	get_device(ctrl->device);
}

static inline void nvme_put_ctrl(struct nvme_ctrl *ctrl)
{
	put_device(ctrl->device);
}

static inline void nvme_end_request(struct request *req, __le16 status,
		union nvme_result result)
{
	struct nvme_request *rq = nvme_req(req);

	rq->status = le16_to_cpu(status) >> 1;
	rq->result = result;
	/* inject error when permitted by fault injection framework */
	nvme_should_fail(req);
	blk_mq_complete_request(req);
}

int nvme_init_identify(struct nvme_ctrl *ctrl);
bool nvme_change_ctrl_state(struct nvme_ctrl *ctrl,
		enum nvme_ctrl_state new_state);
void nvme_complete_rq(struct request *req);
void nvme_cleanup_cmd(struct request *req);
int nvme_disable_ctrl(struct nvme_ctrl *ctrl, u64 cap);
int nvme_enable_ctrl(struct nvme_ctrl *ctrl, u64 cap);
int nvme_init_ctrl(struct nvme_ctrl *ctrl, struct device *dev,
		const struct nvme_ctrl_ops *ops, unsigned long quirks);
int nvme_reset_ctrl(struct nvme_ctrl *ctrl);
void nvme_stop_ctrl(struct nvme_ctrl *ctrl);
void nvme_uninit_ctrl(struct nvme_ctrl *ctrl);
irqreturn_t nvme_irq_check(int irq, void *data);
irqreturn_t nvme_irq(int irq, void *data);
void nvme_complete_async_event(struct nvme_ctrl *ctrl, __le16 status,
		volatile union nvme_result *res);
blk_status_t nvme_setup_cmd(struct nvme_ns *ns, struct request *req,
		struct nvme_command *cmd);


//menvme api
int menvme_init_ctrl(struct nvme_dev *dev, unsigned long quirks);
void menvme_free_ctrl(struct nvme_dev *dev);
int nvme_pci_queue_request_irq(struct nvme_queue *nvmeq);
#endif
