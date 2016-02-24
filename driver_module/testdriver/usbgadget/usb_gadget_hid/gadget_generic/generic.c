#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/hid.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/miscdevice.h>
#include <linux/usb/g_hid.h>
#include <linux/usb/composite.h>

#define GENERIC_NAME "Generic Gadget"
#define TX_REQ_MAX 4
#define ADB_BULK_BUFFER_SIZE           4096


struct generic_dev {
	struct usb_function	function;
	struct usb_composite_dev *cdev;
	spinlock_t lock;

	struct usb_gadget	*gadget;	/* Copy of cdev->gadget */
	struct fsg_common	*common;

	u16			interface_number;

	atomic_t open_excl;

	int online;
	int error;
	unsigned int		bulk_in_enabled:1;
	unsigned int		bulk_out_enabled:1;

	unsigned long		atomic_bitflags;

#define IGNORE_BULK_OUT		0

	struct usb_ep		*ep_in;
	struct usb_ep		*ep_out;

	struct list_head tx_idle;
	wait_queue_head_t read_wq;
	wait_queue_head_t write_wq;
	struct usb_request *rx_req;
	int rx_done;
};

static struct miscdevice gen_device;
static void adb_complete_out(struct usb_ep *ep, struct usb_request *req);
static void adb_complete_in(struct usb_ep *ep, struct usb_request *req);
static struct usb_request *adb_request_new(struct usb_ep *ep, int buffer_size);
static void adb_request_free(struct usb_request *req, struct usb_ep *ep);
struct usb_request *adb_req_get(struct generic_dev *dev, struct list_head *head);
void adb_req_put(struct generic_dev *dev, struct list_head *head,
		struct usb_request *req);

static struct generic_dev *_gen_dev;
static struct usb_interface_descriptor generic_interface_desc = {
	.bLength		= sizeof generic_interface_desc,
	.bDescriptorType	= USB_DT_INTERFACE,
	/* .bInterfaceNumber	= DYNAMIC */
	.bAlternateSetting	= 0,
	.bNumEndpoints		= 2,
	//.bInterfaceClass	= USB_CLASS_HID,
	.bInterfaceClass	= 0,
	/* .bInterfaceSubClass	= DYNAMIC */
	/* .bInterfaceProtocol	= DYNAMIC */
	/* .iInterface		= DYNAMIC */
};

static struct hid_descriptor generic_desc = {
	.bLength			= sizeof generic_desc,
	.bDescriptorType		= HID_DT_HID,
	.bcdHID				= 0x0101,
	.bCountryCode			= 0x00,
	.bNumDescriptors		= 0x1,
	/*.desc[0].bDescriptorType	= DYNAMIC */
	/*.desc[0].wDescriptorLenght	= DYNAMIC */
};

/* High-Speed Support */

static struct usb_endpoint_descriptor generic_hs_in_ep_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	/*.wMaxPacketSize	= DYNAMIC */
	.bInterval		= 4, /* FIXME: Add this field in the
				      * HID gadget configuration?
				      * (struct generic_func_descriptor)
				      */
};

static struct usb_endpoint_descriptor generic_hs_out_ep_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	/*.wMaxPacketSize	= DYNAMIC */
	.bInterval		= 4, /* FIXME: Add this field in the
				      * HID gadget configuration?
				      * (struct generic_func_descriptor)
				      */
};

#if 0
static struct usb_descriptor_header *generic_hs_descriptors[] = {
	(struct usb_descriptor_header *)&hidg_interface_desc,
	(struct usb_descriptor_header *)&hidg_desc,
	(struct usb_descriptor_header *)&hidg_hs_in_ep_desc,
	(struct usb_descriptor_header *)&hidg_hs_out_ep_desc,
	NULL,
};
#else
static struct usb_descriptor_header *generic_hs_descriptors[] = {
	(struct usb_descriptor_header *)&generic_interface_desc,
	(struct usb_descriptor_header *)&generic_hs_in_ep_desc,
	(struct usb_descriptor_header *)&generic_hs_out_ep_desc,
	NULL,
};
#endif

/* Full-Speed Support */

static struct usb_endpoint_descriptor generic_fs_in_ep_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	/*.wMaxPacketSize	= DYNAMIC */
	.bInterval		= 10, /* FIXME: Add this field in the
				       * HID gadget configuration?
				       * (struct generic_func_descriptor)
				       */
};

static struct usb_endpoint_descriptor generic_fs_out_ep_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	/*.wMaxPacketSize	= DYNAMIC */
	.bInterval		= 10, /* FIXME: Add this field in the
				       * HID gadget configuration?
				       * (struct generic_func_descriptor)
				       */
};

static struct usb_descriptor_header *generic_fs_descriptors[] = {
	(struct usb_descriptor_header *)&generic_interface_desc,
	(struct usb_descriptor_header *)&generic_desc,
	(struct usb_descriptor_header *)&generic_fs_in_ep_desc,
	(struct usb_descriptor_header *)&generic_fs_out_ep_desc,
	NULL,
};

static inline struct generic_dev *func_to_gen(struct usb_function *f)
{
	return container_of(f, struct generic_dev, function);
}

static int gen_create_bulk_endpoints(struct generic_dev *dev,
				struct usb_endpoint_descriptor *in_desc,
				struct usb_endpoint_descriptor *out_desc)
{
	struct usb_composite_dev *cdev = dev->cdev;
	struct usb_request *req;
	struct usb_ep *ep;
	int i;

	DBG(cdev, "create_bulk_endpoints dev: %p\n", dev);

	ep = usb_ep_autoconfig(cdev->gadget, in_desc);
	if (!ep) {
		DBG(cdev, "usb_ep_autoconfig for ep_in failed\n");
		return -ENODEV;
	}
	DBG(cdev, "usb_ep_autoconfig for ep_in got %s\n", ep->name);
	ep->driver_data = dev;		/* claim the endpoint */
	dev->ep_in = ep;

	ep = usb_ep_autoconfig(cdev->gadget, out_desc);
	if (!ep) {
		DBG(cdev, "usb_ep_autoconfig for ep_out failed\n");
		return -ENODEV;
	}
	DBG(cdev, "usb_ep_autoconfig for adb ep_out got %s\n", ep->name);
	ep->driver_data = dev;		/* claim the endpoint */
	dev->ep_out = ep;

	/* now allocate requests for our endpoints */
	req = adb_request_new(dev->ep_out, ADB_BULK_BUFFER_SIZE);
	if (!req)
		goto fail;
	req->complete = adb_complete_out;
	dev->rx_req = req;

	for (i = 0; i < TX_REQ_MAX; i++) {
		req = adb_request_new(dev->ep_in, ADB_BULK_BUFFER_SIZE);
		if (!req)
			goto fail;
		req->complete = adb_complete_in;
		adb_req_put(dev, &dev->tx_idle, req);
	}

	return 0;

fail:
	printk(KERN_ERR "adb_bind() could not allocate requests\n");
	return -1;
}

static int __init generic_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	//struct f_generic		*generic = func_to_generic(f);
	int			status;
	struct generic_dev	*dev = func_to_gen(f);

	dev->cdev = cdev;
	/* allocate instance-specific interface IDs, and patch descriptors */
	status = usb_interface_id(c, f);
	if (status < 0)
		goto fail;
	generic_interface_desc.bInterfaceNumber = status;


	/* allocate endpoints */
	status  = gen_create_bulk_endpoints(dev, &generic_fs_in_ep_desc,
			&generic_fs_out_ep_desc);
	if (status)
		return status;
	/* set descriptor dynamic values */
#if 0
	generic_interface_desc.bInterfaceSubClass = generic->bInterfaceSubClass;
	generic_interface_desc.bInterfaceProtocol = generic->bInterfaceProtocol;
#endif
	generic_hs_in_ep_desc.wMaxPacketSize = cpu_to_le16(64);
	generic_fs_in_ep_desc.wMaxPacketSize = cpu_to_le16(64);
	generic_hs_out_ep_desc.wMaxPacketSize = cpu_to_le16(64);
	generic_fs_out_ep_desc.wMaxPacketSize = cpu_to_le16(64);
	//generic_desc.desc[0].bDescriptorType = HID_DT_REPORT;
	generic_desc.desc[0].wDescriptorLength =
		cpu_to_le16(64);

	generic_hs_in_ep_desc.bEndpointAddress =
		generic_fs_in_ep_desc.bEndpointAddress;
	generic_hs_out_ep_desc.bEndpointAddress =
		generic_fs_out_ep_desc.bEndpointAddress;

	status = usb_assign_descriptors(f, generic_fs_descriptors,
			generic_hs_descriptors, NULL);
	if (status)
		goto fail;

	return 0;

fail:
	ERROR(f->config->cdev, "generic_bind FAILED\n");
	usb_free_all_descriptors(f);
	return status;
}

static void generic_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct generic_dev	*dev = func_to_gen(f);
	struct usb_request *req;

	wake_up(&dev->read_wq);

	adb_request_free(dev->rx_req, dev->ep_out);
	while ((req = adb_req_get(dev, &dev->tx_idle)))
		adb_request_free(req, dev->ep_in);

	misc_deregister(&gen_device);
	kfree(_gen_dev);
	_gen_dev = NULL;
}


static inline int adb_lock(atomic_t *excl)
{
	if (atomic_inc_return(excl) == 1) {
		return 0;
	} else {
		atomic_dec(excl);
		return -1;
	}
}

static inline void adb_unlock(atomic_t *excl)
{
	atomic_dec(excl);
}
static struct usb_request *adb_request_new(struct usb_ep *ep, int buffer_size)
{
	struct usb_request *req = usb_ep_alloc_request(ep, GFP_KERNEL);
	if (!req)
		return NULL;

	/* now allocate buffers for the requests */
	req->buf = kmalloc(buffer_size, GFP_KERNEL);
	if (!req->buf) {
		usb_ep_free_request(ep, req);
		return NULL;
	}

	return req;
}

static void adb_request_free(struct usb_request *req, struct usb_ep *ep)
{
	if (req) {
		kfree(req->buf);
		usb_ep_free_request(ep, req);
	}
}

/* remove a request from the head of a list */
struct usb_request *adb_req_get(struct generic_dev *dev, struct list_head *head)
{
	unsigned long flags;
	struct usb_request *req;

	spin_lock_irqsave(&dev->lock, flags);
	if (list_empty(head)) {
		req = 0;
	} else {
		req = list_first_entry(head, struct usb_request, list);
		list_del(&req->list);
	}
	spin_unlock_irqrestore(&dev->lock, flags);
	return req;
}

/* add a request to the tail of a list */
void adb_req_put(struct generic_dev *dev, struct list_head *head,
		struct usb_request *req)
{
	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);
	list_add_tail(&req->list, head);
	spin_unlock_irqrestore(&dev->lock, flags);
}

static void adb_complete_in(struct usb_ep *ep, struct usb_request *req)
{
	struct generic_dev *dev = _gen_dev;

	if (req->status != 0)
		dev->error = 1;

	adb_req_put(dev, &dev->tx_idle, req);

	wake_up(&dev->write_wq);
}

static void adb_complete_out(struct usb_ep *ep, struct usb_request *req)
{
	struct generic_dev *dev = _gen_dev;

	dev->rx_done = 1;
	if (req->status != 0)
		dev->error = 1;

	wake_up(&dev->read_wq);
}

static ssize_t adb_read(struct file *fp, char __user *buf,
				size_t count, loff_t *pos)
{
	return 0;
}

static ssize_t adb_write(struct file *fp, const char __user *buf,
				 size_t count, loff_t *pos)
{
	struct usb_request *req = 0;
	struct generic_dev *dev = fp->private_data;
	int r = count, xfer, ret;

	while (count > 0) {
		ret = wait_event_interruptible(dev->write_wq,
			(req = adb_req_get(dev, &dev->tx_idle)) || dev->error);

		if (ret < 0) {
			r = ret;
			break;
		}

		if (req != 0) {
			if (count > ADB_BULK_BUFFER_SIZE)
				xfer = ADB_BULK_BUFFER_SIZE;
			else
				xfer = count;

			if (copy_from_user(req->buf, buf, xfer)) {
				r = -EFAULT;
				break;
			}

			ret = usb_ep_queue(dev->ep_in, req, GFP_ATOMIC);
			if (ret < 0) {
				pr_debug("adb_write: xfer error %d\n", ret);
				dev->error = 1;
				r = -EIO;
				break;
			}

			buf += xfer;
			count -= xfer;

			/* zero this so we don't try to free it on error exit */
			req = 0;
		}
	}

	if (req)
		adb_req_put(dev, &dev->tx_idle, req);

	return r;
}

static int adb_open(struct inode *ip, struct file *fp)
{
	if (!_gen_dev)
		return -ENODEV;

	if (adb_lock(&_gen_dev->open_excl))
		return -EBUSY;

	fp->private_data = _gen_dev;
	/* clear the error latch */
	_gen_dev->error = 0;
	return 0;
}

static int adb_release(struct inode *ip, struct file *fp)
{
	return 0;
}

/* file operations for ADB device /dev/android_adb */
static struct file_operations gen_fops = {
	.owner = THIS_MODULE,
	.read = adb_read,
	.write = adb_write,
	.open = adb_open,
	.release = adb_release,
};

static const char gen_shortname[] = "gen_dev";
static struct miscdevice gen_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = gen_shortname,
	.fops = &gen_fops,
};

static int generic_setup(struct usb_function *f,
		const struct usb_ctrlrequest *ctrl)
{
#if 0
	struct generic_dev *dev;
	int ret;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	spin_lock_init(&dev->lock);
	init_waitqueue_head(&dev->read_wq);
	init_waitqueue_head(&dev->write_wq);

	INIT_LIST_HEAD(&dev->tx_idle);

	ret = misc_register(&gen_device);

	if (ret)
		goto err;

	return 0;

err:
	kfree(dev);
	printk(KERN_ERR "adb gadget driver failed to initialize\n");
	return ret;
#endif
	return 0;
}

static int generic_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	return 0;
}

static void generic_disable(struct usb_function *f)
{
	struct generic_dev	*dev = func_to_gen(f);
	struct usb_composite_dev	*cdev = dev->cdev;

	DBG(cdev, "adb_function_disable cdev %p\n", cdev);
	dev->online = 0;
	dev->error = 1;
	usb_ep_disable(dev->ep_in);
	usb_ep_disable(dev->ep_out);

	/* readers may be blocked waiting for us to go online */
	wake_up(&dev->read_wq);

	VDBG(cdev, "%s disabled\n", dev->function.name);
}

static void generic_free(struct usb_function *f)
{

}

static int generic_bind_config(struct usb_configuration *c)
{
	struct generic_dev *dev;
	int status;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->function.name	= GENERIC_NAME;
	dev->function.bind	= generic_bind;
	dev->function.unbind	= generic_unbind;
	dev->function.setup	= generic_setup;
	dev->function.set_alt	= generic_set_alt;
	dev->function.disable	= generic_disable;
	dev->function.free_func	= generic_free;

	_gen_dev = dev;

	if (status)
		kfree(dev);

	spin_lock_init(&dev->lock);

	atomic_set(&dev->open_excl, 0);

	init_waitqueue_head(&dev->read_wq);
	init_waitqueue_head(&dev->write_wq);

	INIT_LIST_HEAD(&dev->tx_idle);

	status = misc_register(&gen_device);

	if (status)
		goto err;

	status = usb_add_function(c, &dev->function);
	return status;

err:
	usb_remove_function(c, &dev->function);
	kfree(dev);
	return status;
}
//DECLARE_USB_FUNCTION_INIT(mass_storage, fsg_alloc_inst, fsg_alloc);
