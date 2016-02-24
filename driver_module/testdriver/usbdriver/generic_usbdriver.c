#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/mutex.h>

#include <asm/uaccess.h>
#include <linux/input.h>
#include <linux/usb.h>
#include <linux/poll.h>

#ifdef CONFIG_USB_DYNAMIC_MINORS
#define USB_LD_MINOR_BASE	0
#else
#define USB_LD_MINOR_BASE	176
#endif
/* Structure to hold all of our device specific stuff */
struct ld_usb {
	struct mutex		mutex;		/* locks this structure */
	struct usb_interface*	intf;		/* save off the usb interface pointer */

	int			open_count;	/* number of times this port has been opened */

	char*			ring_buffer;
	unsigned int		ring_head;
	unsigned int		ring_tail;

	wait_queue_head_t	read_wait;
	wait_queue_head_t	write_wait;

	char*			blk_in_buffer;
	struct usb_endpoint_descriptor* blk_in_endpoint;
	struct urb*		blk_in_urb;
	size_t			blk_in_endpoint_size;
	int			blk_in_running;
	int			blk_in_done;
	int			buffer_overflow;
	spinlock_t		rbsl;

	char*			blk_out_buffer;
	struct usb_endpoint_descriptor* blk_out_endpoint;
	struct urb*		blk_out_urb;
	size_t			blk_out_endpoint_size;
	int			blk_out_busy;
};

static int write_buffer_size = 10;
static int ring_buffer_size = 128;

static struct usb_driver ld_usb_driver;
static void ld_usb_abort_transfers(struct ld_usb *dev);
static void ld_usb_delete(struct ld_usb *dev);
/**
 *	ld_usb_interrupt_in_callback
 */
static void ld_usb_blk_in_callback(struct urb *urb)
{
	struct ld_usb *dev = urb->context;
	size_t *actual_buffer;
	unsigned int next_ring_head;
	int status = urb->status;
	int retval;

	if (status) {
		if (status == -ENOENT ||
		    status == -ECONNRESET ||
		    status == -ESHUTDOWN) {
			goto exit;
		} else {
			dev_dbg(&dev->intf->dev,
				"%s: nonzero status received: %d\n", __func__,
				status);
			spin_lock(&dev->rbsl);
			goto resubmit; /* maybe we can recover */
		}
	}

	spin_lock(&dev->rbsl);
	if (urb->actual_length > 0) {
		next_ring_head = (dev->ring_head+1) % ring_buffer_size;
		if (next_ring_head != dev->ring_tail) {
			actual_buffer = (size_t*)(dev->ring_buffer + dev->ring_head*(sizeof(size_t)+dev->blk_in_endpoint_size));
			/* actual_buffer gets urb->actual_length + blk_in_buffer */
			*actual_buffer = urb->actual_length;
			memcpy(actual_buffer+1, dev->blk_in_buffer, urb->actual_length);
			dev->ring_head = next_ring_head;
			dev_dbg(&dev->intf->dev, "%s: received %d bytes\n",
				__func__, urb->actual_length);
		} else {
			dev_warn(&dev->intf->dev,
				 "Ring buffer overflow, %d bytes dropped\n",
				 urb->actual_length);
			dev->buffer_overflow = 1;
		}
	}

resubmit:
	/* resubmit if we're still running */
	if (dev->blk_in_running && !dev->buffer_overflow && dev->intf) {
		retval = usb_submit_urb(dev->blk_in_urb, GFP_ATOMIC);
		if (retval) {
			dev_err(&dev->intf->dev,
				"usb_submit_urb failed (%d)\n", retval);
			dev->buffer_overflow = 1;
		}
	}
	spin_unlock(&dev->rbsl);
exit:
	dev->blk_in_done = 1;
	wake_up_interruptible(&dev->read_wait);
}
/**
 *	ld_usb_interrupt_out_callback
 */
static void ld_usb_blk_out_callback(struct urb *urb)
{
	struct ld_usb *dev = urb->context;
	int status = urb->status;

	/* sync/async unlink faults aren't errors */
	if (status && !(status == -ENOENT ||
			status == -ECONNRESET ||
			status == -ESHUTDOWN))
		dev_dbg(&dev->intf->dev,
			"%s - nonzero write blk status received: %d\n",
			__func__, status);

	dev->blk_out_busy = 0;
	wake_up_interruptible(&dev->write_wait);
}
/**
 *	ld_usb_poll
 */
static unsigned int ld_usb_poll(struct file *file, poll_table *wait)
{
	struct ld_usb *dev;
	unsigned int mask = 0;

	dev = file->private_data;

	if (!dev->intf)
		return POLLERR | POLLHUP;

	poll_wait(file, &dev->read_wait, wait);
	poll_wait(file, &dev->write_wait, wait);

	if (dev->ring_head != dev->ring_tail)
		mask |= POLLIN | POLLRDNORM;
	if (!dev->blk_out_busy)
		mask |= POLLOUT | POLLWRNORM;

	return mask;
}

/**
 *	ld_usb_release
 */
static int ld_usb_release(struct inode *inode, struct file *file)
{
	struct ld_usb *dev;
	int retval = 0;

	dev = file->private_data;

	if (dev == NULL) {
		retval = -ENODEV;
		goto exit;
	}

	if (mutex_lock_interruptible(&dev->mutex)) {
		retval = -ERESTARTSYS;
		goto exit;
	}

	if (dev->open_count != 1) {
		retval = -ENODEV;
		goto unlock_exit;
	}
	if (dev->intf == NULL) {
		/* the device was unplugged before the file was released */
		mutex_unlock(&dev->mutex);
		/* unlock here as ld_usb_delete frees dev */
		ld_usb_delete(dev);
		goto exit;
	}

	/* wait until write transfer is finished */
	if (dev->blk_out_busy)
		wait_event_interruptible_timeout(dev->write_wait, !dev->blk_out_busy, 2 * HZ);
	ld_usb_abort_transfers(dev);
	dev->open_count = 0;

unlock_exit:
	mutex_unlock(&dev->mutex);

exit:
	return retval;
	return 0;	
}

/**
 *	ld_usb_open
 */
static int ld_usb_open(struct inode *inode, struct file *file)
{
	struct ld_usb *dev;
	int subminor;
	int retval;
	struct usb_interface *interface;

	nonseekable_open(inode, file);
	subminor = iminor(inode);

	interface = usb_find_interface(&ld_usb_driver, subminor);
	if (!interface) {
		printk(KERN_ERR "%s - error, can't find device for minor %d\n",
		       __func__, subminor);
		return -ENODEV;
	}

	dev = usb_get_intfdata(interface);

	if (!dev)
		return -ENODEV;

	/* lock this device */
	if (mutex_lock_interruptible(&dev->mutex))
		return -ERESTARTSYS;

	/* allow opening only once */
	if (dev->open_count) {
		retval = -EBUSY;
		goto unlock_exit;
	}
	dev->open_count = 1;

	/* initialize in direction */
	dev->ring_head = 0;
	dev->ring_tail = 0;
	dev->buffer_overflow = 0;

	usb_fill_bulk_urb(dev->blk_in_urb,
			 interface_to_usbdev(interface),
			 usb_rcvintpipe(interface_to_usbdev(interface),
					dev->blk_in_endpoint->bEndpointAddress),
			 dev->blk_in_buffer,
			 dev->blk_in_endpoint_size,
			 ld_usb_blk_in_callback,
			 dev);

	dev->blk_in_running = 1;
	dev->blk_in_done = 0;

	retval = usb_submit_urb(dev->blk_in_urb, GFP_KERNEL);
	if (retval) {
		dev_err(&interface->dev, "Couldn't submit interrupt_in_urb %d\n", retval);
		dev->blk_in_running = 0;
		dev->open_count = 0;
		goto unlock_exit;
	}

	/* save device in the file's private structure */
	file->private_data = dev;

unlock_exit:
	mutex_unlock(&dev->mutex);

	return retval;

}

/**
 *	ld_usb_write
 */
static ssize_t ld_usb_write(struct file *file, const char __user *buffer,
			    size_t count, loff_t *ppos)
{
	struct ld_usb *dev;
	size_t bytes_to_write;
	int retval = 0;

	dev = file->private_data;

	/* verify that we actually have some data to write */
	if (count == 0)
		goto exit;

	/* lock this object */
	if (mutex_lock_interruptible(&dev->mutex)) {
		retval = -ERESTARTSYS;
		goto exit;
	}

	/* verify that the device wasn't unplugged */
	if (dev->intf == NULL) {
		retval = -ENODEV;
		printk(KERN_ERR "ldusb: No device or device unplugged %d\n", retval);
		goto unlock_exit;
	}

	/* wait until previous transfer is finished */
	if (dev->blk_out_busy) {
		if (file->f_flags & O_NONBLOCK) {
			retval = -EAGAIN;
			goto unlock_exit;
		}
		retval = wait_event_interruptible(dev->write_wait, !dev->blk_out_busy);
		if (retval < 0) {
			goto unlock_exit;
		}
	}

	/* write the data into interrupt_out_buffer from userspace */
	bytes_to_write = min(count, write_buffer_size*dev->blk_out_endpoint_size);
	if (bytes_to_write < count)
		dev_warn(&dev->intf->dev, "Write buffer overflow, %zd bytes dropped\n",count-bytes_to_write);
	dev_dbg(&dev->intf->dev, "%s: count = %zd, bytes_to_write = %zd\n",
		__func__, count, bytes_to_write);

	if (copy_from_user(dev->blk_out_buffer, buffer, bytes_to_write)) {
		retval = -EFAULT;
		goto unlock_exit;
	}

	if (dev->blk_out_endpoint == NULL) {
		/* try HID_REQ_SET_REPORT=9 on control_endpoint instead of interrupt_out_endpoint */
		retval = usb_control_msg(interface_to_usbdev(dev->intf),
					 usb_sndctrlpipe(interface_to_usbdev(dev->intf), 0),
					 9,
					 USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_OUT,
					 1 << 8, 0,
					 dev->blk_out_buffer,
					 bytes_to_write,
					 USB_CTRL_SET_TIMEOUT * HZ);
		if (retval < 0)
			dev_err(&dev->intf->dev,
				"Couldn't submit HID_REQ_SET_REPORT %d\n",
				retval);
		goto unlock_exit;
	}

	/* send off the urb */
	usb_fill_bulk_urb(dev->blk_out_urb,
			 interface_to_usbdev(dev->intf),
			 usb_sndintpipe(interface_to_usbdev(dev->intf),
					dev->blk_out_endpoint->bEndpointAddress),
			 dev->blk_out_buffer,
			 bytes_to_write,
			 ld_usb_blk_out_callback,
			 dev);
			 

	dev->blk_out_busy = 1;
	wmb();

	retval = usb_submit_urb(dev->blk_out_urb, GFP_KERNEL);
	if (retval) {
		dev->blk_out_busy = 0;
		dev_err(&dev->intf->dev,
			"Couldn't submit blk_out_urb %d\n", retval);
		goto unlock_exit;
	}
	retval = bytes_to_write;

unlock_exit:
	/* unlock the device */
	mutex_unlock(&dev->mutex);

exit:
	return retval;
}
/**
 *	ld_usb_read
 */
static ssize_t ld_usb_read(struct file *file, char __user *buffer, size_t count,
			   loff_t *ppos)
{
	struct ld_usb *dev;
	size_t *actual_buffer;
	size_t bytes_to_read;
	int retval = 0;
	int rv;

	dev = file->private_data;

	/* verify that we actually have some data to read */
	if (count == 0)
		goto exit;

	/* lock this object */
	if (mutex_lock_interruptible(&dev->mutex)) {
		retval = -ERESTARTSYS;
		goto exit;
	}

	/* verify that the device wasn't unplugged */
	if (dev->intf == NULL) {
		retval = -ENODEV;
		printk(KERN_ERR "ldusb: No device or device unplugged %d\n", retval);
		goto unlock_exit;
	}

	/* wait for data */
	spin_lock_irq(&dev->rbsl);
	if (dev->ring_head == dev->ring_tail) {
		dev->blk_in_done = 0;
		spin_unlock_irq(&dev->rbsl);
		if (file->f_flags & O_NONBLOCK) {
			retval = -EAGAIN;
			goto unlock_exit;
		}
		retval = wait_event_interruptible(dev->read_wait, dev->blk_in_done);
		if (retval < 0)
			goto unlock_exit;
	} else {
		spin_unlock_irq(&dev->rbsl);
	}

	/* actual_buffer contains actual_length + interrupt_in_buffer */
	actual_buffer = (size_t*)(dev->ring_buffer + dev->ring_tail*(sizeof(size_t)+dev->blk_in_endpoint_size));
	bytes_to_read = min(count, *actual_buffer);
	if (bytes_to_read < *actual_buffer)
		dev_warn(&dev->intf->dev, "Read buffer overflow, %zd bytes dropped\n",
			 *actual_buffer-bytes_to_read);

	/* copy one blk_in_buffer from ring_buffer into userspace */
	if (copy_to_user(buffer, actual_buffer+1, bytes_to_read)) {
		retval = -EFAULT;
		goto unlock_exit;
	}
	dev->ring_tail = (dev->ring_tail+1) % ring_buffer_size;

	retval = bytes_to_read;

	spin_lock_irq(&dev->rbsl);
	if (dev->buffer_overflow) {
		dev->buffer_overflow = 0;
		spin_unlock_irq(&dev->rbsl);
		rv = usb_submit_urb(dev->blk_in_urb, GFP_KERNEL);
		if (rv < 0)
			dev->buffer_overflow = 1;
	} else {
		spin_unlock_irq(&dev->rbsl);
	}

unlock_exit:
	/* unlock the device */
	mutex_unlock(&dev->mutex);

exit:
	return retval;
	return 0;
}

/* file operations needed when we register this driver */
static const struct file_operations ld_usb_fops = {
	.owner =	THIS_MODULE,
	.read  =	ld_usb_read,
	.write =	ld_usb_write,
	.open =		ld_usb_open,
	.release =	ld_usb_release,
	.poll =		ld_usb_poll,
	.llseek =	no_llseek,
};

static struct usb_class_driver ld_usb_class = {
	.name =		"ldusb%d",
	.fops =		&ld_usb_fops,
	.minor_base =	USB_LD_MINOR_BASE,
};

/* table of devices that work with this driver */
static const struct usb_device_id ld_usb_table[] = {
	{ USB_DEVICE(0x0d6b, 0x0106) },
	{ }					/* Terminating entry */
};
MODULE_DEVICE_TABLE(usb, ld_usb_table);

/**
 *	ld_usb_abort_transfers
 *      aborts transfers and frees associated data structures
 */
static void ld_usb_abort_transfers(struct ld_usb *dev)
{
	/* shutdown transfer */
	if (dev->blk_in_running) {
		dev->blk_in_running = 0;
		if (dev->intf)
			usb_kill_urb(dev->blk_in_urb);
	}
	if (dev->blk_out_busy)
		if (dev->intf)
			usb_kill_urb(dev->blk_out_urb);
}

/**
 *	ld_usb_delete
 */
static void ld_usb_delete(struct ld_usb *dev)
{
	ld_usb_abort_transfers(dev);

	/* free data structures */
	usb_free_urb(dev->blk_in_urb);
	usb_free_urb(dev->blk_out_urb);
	kfree(dev->ring_buffer);
	kfree(dev->blk_in_buffer);
	kfree(dev->blk_out_buffer);
	kfree(dev);
}

static int ld_usb_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_device *udev = interface_to_usbdev(intf);
	struct ld_usb *dev = NULL;
	struct usb_host_interface *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	int retval = 0, i;

	/* allocate memory for our device state and initialize it */
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (dev == NULL) {
		dev_err(&intf->dev, "Out of memory\n");
		goto exit;
	}

	mutex_init(&dev->mutex);
	spin_lock_init(&dev->rbsl);
	dev->intf = intf;
	init_waitqueue_head(&dev->read_wait);
	init_waitqueue_head(&dev->write_wait);

	iface_desc = intf->cur_altsetting;
	for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
		endpoint = &iface_desc->endpoint[i].desc;

		if (usb_endpoint_is_bulk_in(endpoint))
			dev->blk_in_endpoint = endpoint;

		if (usb_endpoint_is_bulk_out(endpoint))
			dev->blk_out_endpoint = endpoint;
	}

	dev->blk_in_endpoint_size = usb_endpoint_maxp(dev->blk_in_endpoint);
	dev->ring_buffer = kmalloc(ring_buffer_size*(sizeof(size_t)+dev->blk_in_endpoint_size), GFP_KERNEL);

	dev->blk_in_buffer = kmalloc(dev->blk_in_endpoint_size, GFP_KERNEL);
	dev->blk_in_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!dev->blk_in_urb) {
		dev_err(&intf->dev, "Couldn't allocate blk_in_urb\n");
		goto error;
	}

	dev->blk_out_endpoint_size = dev->blk_out_endpoint ? usb_endpoint_maxp(dev->blk_out_endpoint) : udev->descriptor.bMaxPacketSize0;
	dev->blk_out_buffer = kmalloc(write_buffer_size*dev->blk_out_endpoint_size, GFP_KERNEL);

	if (!dev->blk_out_buffer) {
		dev_err(&intf->dev, "Couldn't allocate blk_out_buffer\n");
		goto error;
	}

	dev->blk_out_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!dev->blk_out_urb) {
		dev_err(&intf->dev, "Couldn't allocate blk_out_urb\n");
		goto error;
	}

	/* we can register the device now, as it is ready */
	usb_set_intfdata(intf, dev);

	retval = usb_register_dev(intf, &ld_usb_class);
	if (retval) {
		/* something prevented us from registering this driver */
		dev_err(&intf->dev, "Not able to get a minor for this device.\n");
		usb_set_intfdata(intf, NULL);
		goto error;
	}

	return 0;
exit:
	return retval;
error:
	ld_usb_delete(dev);

	return retval;
}

/**
 *	ld_usb_disconnect
 *
 *	Called by the usb core when the device is removed from the system.
 */
static void ld_usb_disconnect(struct usb_interface *intf)
{
	struct ld_usb *dev;
	int minor;

	dev = usb_get_intfdata(intf);
	usb_set_intfdata(intf, NULL);

	/* give back our minor */
	usb_deregister_dev(intf, &ld_usb_class);

	mutex_lock(&dev->mutex);
	/* if the device is not opened, then we clean up right now */
	if (!dev->open_count) {
		mutex_unlock(&dev->mutex);
		ld_usb_delete(dev);
	} else {
		dev->intf = NULL;
		/* wake up pollers */
		wake_up_interruptible_all(&dev->read_wait);
		wake_up_interruptible_all(&dev->write_wait);
		mutex_unlock(&dev->mutex);
	}

	dev_info(&intf->dev, "LD USB Device #%d now disconnected\n",
		 (minor - USB_LD_MINOR_BASE));
}

/* usb specific object needed to register this driver with the usb subsystem */
static struct usb_driver ld_usb_driver = {
	.name =		"ldusb",
	.probe =	ld_usb_probe,
	.disconnect =	ld_usb_disconnect,
	.id_table =	ld_usb_table,
};

module_usb_driver(ld_usb_driver);
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("LD USB Devices");

