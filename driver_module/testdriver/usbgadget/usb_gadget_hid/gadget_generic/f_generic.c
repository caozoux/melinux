#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/usb/composite.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/usb/g_hid.h>
#include <linux/usb/composite.h> 
//#include "u_serial.h"
#include "gadget_chips.h"
#include "generic.c"

#define DRIVER_DESC		"Generic Gadget"
#define DRIVER_VERSION	generic	"2010/03/16"

#define VENDOR_NUM	0x0d6b	/* Linux Foundation */
#define PRODUCT_NUM	0x0106	/* Composite Gadget: ACM + MS*/
USB_GADGET_COMPOSITE_OPTIONS();

static struct usb_device_descriptor device_desc = {
	.bLength =		sizeof device_desc,
	.bDescriptorType =	USB_DT_DEVICE,

	.bcdUSB =		cpu_to_le16(0x0200),

	.bDeviceClass =		USB_CLASS_MISC /* 0xEF */,
	.bDeviceSubClass =	2,
	.bDeviceProtocol =	1,

	/* .bMaxPacketSize0 = f(hardware) */

	/* Vendor and product id can be overridden by module parameters.  */
	.idVendor =		cpu_to_le16(VENDOR_NUM),
	.idProduct =		cpu_to_le16(PRODUCT_NUM),
	/* .bcdDevice = f(hardware) */
	/* .iManufacturer = DYNAMIC */
	/* .iProduct = DYNAMIC */
	/* NO SERIAL NUMBER */
	/*.bNumConfigurations =	DYNAMIC*/
};

static struct usb_otg_descriptor otg_descriptor = {
	.bLength =		sizeof otg_descriptor,
	.bDescriptorType =	USB_DT_OTG,

	/*
	 * REVISIT SRP-only hardware is possible, although
	 * it would not be called "OTG" ...
	 */
	.bmAttributes =		USB_OTG_SRP | USB_OTG_HNP,
};

static const struct usb_descriptor_header *otg_desc[] = {
	(struct usb_descriptor_header *) &otg_descriptor,
	NULL,
};

/* string IDs are assigned dynamically */
static struct usb_string strings_dev[] = {
	[USB_GADGET_MANUFACTURER_IDX].s = "",
	[USB_GADGET_PRODUCT_IDX].s = DRIVER_DESC,
	[USB_GADGET_SERIAL_IDX].s = "",
	{  } /* end of list */
};

static struct usb_gadget_strings stringtab_dev = {
	.language	= 0x0409,	/* en-us */
	.strings	= strings_dev,
};

static struct usb_gadget_strings *dev_strings[] = {
	&stringtab_dev,
	NULL,
};

static int __init do_config(struct usb_configuration *c)
{
	int status = 0;

	if (gadget_is_otg(c->cdev->gadget)) {
		c->descriptors = otg_desc;
		c->bmAttributes |= USB_CONFIG_ATT_WAKEUP;
	}

#if 0
	list_for_each_entry(e, &hidg_func_list, node) {
		status = hidg_bind_config(c, e->func, func++);
		if (status)
			break;
	}
#endif

	generic_bind_config(c);
	return status;
}

static struct usb_configuration config_driver = {
	.label			= "generic Gadget",
	.bConfigurationValue	= 1,
	/* .iConfiguration = DYNAMIC */
	.bmAttributes		= USB_CONFIG_ATT_SELFPOWER,
};

static int __init f_generic_bind(struct usb_composite_dev *cdev)
{
	int status;
	status = usb_string_ids_tab(cdev, strings_dev);
	if (status < 0)
		return status;
	/* register our configuration */
	device_desc.iManufacturer = strings_dev[USB_GADGET_MANUFACTURER_IDX].id;
	device_desc.iProduct = strings_dev[USB_GADGET_PRODUCT_IDX].id;
	status = usb_add_config(cdev, &config_driver, do_config);
	if (status < 0)
		return status;

	usb_composite_overwrite_options(cdev, &coverwrite);
	return 0;
}

static int __exit f_generic_unbind(struct usb_composite_dev *cdev)
{
	return 0;
}

static __refdata struct usb_composite_driver generic_driver = {
	.name		= "f_generic",
	.dev		= &device_desc,
	.max_speed	= USB_SPEED_SUPER,
	.strings	= dev_strings,
	.bind		= f_generic_bind,
	.unbind		= __exit_p(f_generic_unbind),
};

static int __init init(void)
{
	return usb_composite_probe(&generic_driver);
}
module_init(init);

static void __exit cleanup(void)
{
	usb_composite_unregister(&generic_driver);
}
module_exit(cleanup);
//DECLARE_USB_FUNCTION_INIT(acm, acm_alloc_instance, acm_alloc_func);
MODULE_LICENSE("GPL");
