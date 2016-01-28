#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/usb/g_hid.h>
#include <linux/usb/composite.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/usb/g_hid.h>

#define DRIVER_DESC		"HID Gadget"
#define DRIVER_VERSION		"2010/03/16"

static int hid_drv_probe(struct hid_device *hdev, const struct hid_device_id *id)
{
	return 0;
}

static const struct hid_device_id hid_drv_devices[] = {
	{ HID_USB_DEVICE(USB_VENDOR_ID_A4TECH, USB_DEVICE_ID_A4TECH_WCP32PU),
		.driver_data = A4_2WHEEL_MOUSE_HACK_7 },
	{ HID_USB_DEVICE(USB_VENDOR_ID_A4TECH, USB_DEVICE_ID_A4TECH_X5_005D),
		.driver_data = A4_2WHEEL_MOUSE_HACK_B8 },
	{ HID_USB_DEVICE(USB_VENDOR_ID_A4TECH, USB_DEVICE_ID_A4TECH_RP_649),
		.driver_data = A4_2WHEEL_MOUSE_HACK_B8 },
	{ }
};
MODULE_DEVICE_TABLE(hid, hid_drv_devices);
static struct hid_driver hid_drv_driver = {
	.name = "a4tech",
	.id_table = a4_devices,
	.input_mapped = a4_input_mapped,
	.event = a4_event,
	.probe = a4_probe,
};

module_hid_driver(hid_drv_driver);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_AUTHOR("Fabien Chouteau, Peter Korsgaard");
MODULE_LICENSE("GPL");

