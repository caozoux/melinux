#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/hid.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/usb/g_hid.h>
#include <linux/usb/composite.h>

#define GENERIC_NAME "Generic Gadget"
struct generic_dev {
	struct usb_function	function;
	struct usb_gadget	*gadget;	/* Copy of cdev->gadget */
	struct fsg_common	*common;

	u16			interface_number;

	unsigned int		bulk_in_enabled:1;
	unsigned int		bulk_out_enabled:1;

	unsigned long		atomic_bitflags;
#define IGNORE_BULK_OUT		0

	struct usb_ep		*bulk_in;
	struct usb_ep		*bulk_out;
};

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

static int __init generic_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_ep		*ep;
	//struct f_generic		*generic = func_to_generic(f);
	int			status;
	struct usb_request *req;

	/* allocate instance-specific interface IDs, and patch descriptors */
	status = usb_interface_id(c, f);
	if (status < 0)
		goto fail;
	generic_interface_desc.bInterfaceNumber = status;

	/* allocate instance-specific endpoints */
	status = -ENODEV;
	ep = usb_ep_autoconfig(c->cdev->gadget, &generic_fs_in_ep_desc);
	if (!ep)
		goto fail;
	ep->driver_data = c->cdev;	/* claim */
	//generic->in_ep = ep;

	ep = usb_ep_autoconfig(c->cdev->gadget, &generic_fs_out_ep_desc);
	if (!ep)
		goto fail;
	ep->driver_data = c->cdev;	/* claim */
	//generic->out_ep = ep;

	/* preallocate request and buffer */
	status = -ENOMEM;
	req = usb_ep_alloc_request(ep, GFP_KERNEL);
	if (!req)
		goto fail;

	req->buf = kmalloc(64, GFP_KERNEL);
	if (!req->buf)
		goto fail;

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

#if 0
	mutex_init(&generic->lock);
	spin_lock_init(&hidg->spinlock);
	init_waitqueue_head(&hidg->write_queue);
	init_waitqueue_head(&hidg->read_queue);
	INIT_LIST_HEAD(&hidg->completed_out_req);

	/* create char device */
	cdev_init(&hidg->cdev, &f_hidg_fops);
	dev = MKDEV(major, hidg->minor);
	status = cdev_add(&hidg->cdev, dev, 1);
	if (status)
		goto fail;

	device_create(hidg_class, NULL, dev, NULL, "%s%d", "hidg", hidg->minor);
#endif

	return 0;

fail:
	ERROR(f->config->cdev, "generic_bind FAILED\n");
	if (req != NULL) {
		kfree(req->buf);
		if (ep != NULL)
			usb_ep_free_request(ep, req);
	}

	usb_free_all_descriptors(f);
	return status;
}

static void generic_unbind(struct usb_configuration *c, struct usb_function *f)
{
#if 0
	struct f_hidg *hidg = func_to_hidg(f);

	//cdev_del(&hidg->cdev);

	/* disable/free request and end point */
	usb_ep_disable(hidg->in_ep);
	usb_ep_dequeue(hidg->in_ep, hidg->req);
	//kfree(hidg->req->buf);
	usb_ep_free_request(hidg->in_ep, hidg->req);

	usb_free_all_descriptors(f);
#endif

}

static int generic_setup(struct usb_function *f,
		const struct usb_ctrlrequest *ctrl)
{
	return 0;
}

static int generic_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	return 0;
}

static void generic_disable(struct usb_function *f)
{

}

static int generic_bind_config(struct usb_configuration *c)
{
	struct generic_dev *gen_d;
	int status;
	gen_d = kzalloc(sizeof(struct generic_dev), GFP_KERNEL);
	if (unlikely(!gen_d))
		return -ENOMEM;
	gen_d->function.name	= GENERIC_NAME;
	gen_d->function.bind	= generic_bind;
	gen_d->function.unbind	= generic_unbind;
	gen_d->function.setup	= generic_setup;
	gen_d->function.set_alt	= generic_set_alt;
	gen_d->function.disable	= generic_disable;
	//gen_d->function.free_func	= generic_free;
	status = usb_add_function(c, &gen_d->function);

	if (status)
		kfree(gen_d);

	return status;
}
//DECLARE_USB_FUNCTION_INIT(mass_storage, fsg_alloc_inst, fsg_alloc);

