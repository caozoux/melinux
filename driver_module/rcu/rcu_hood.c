#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/syscore_ops.h>
#include <linux/syscalls.h>
#include <linux/miscdevice.h>
#include  "rcu_module.h"

static void * gbl_foo;
static void *rcu_pointer;
static spinlock_t	hook_rcu_spinlock;
static long rcu_hook_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg);


static const struct file_operations rcu_hook_ops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = rcu_hook_unlocked_ioctl,
};

static struct miscdevice rcu_hook_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "rcu_hood",
	.fops = &rcu_hook_ops,
};

static void rcu_write(int sync)
{
#if 0
	int *new_fp; 
	int *old_fp;

	new_fp = kmalloc(1024, GFP_KERNEL); 
	old_fp = gbl_foo; 
	*new_fp = *old_fp; 
	rcu_assign_pointer(gbl_foo, new_fp); 
	//printk("zz %s %d \n", __func__, __LINE__);
	//synchronize_rcu();
	//printk("zz %s %d \n", __func__, __LINE__);
	kfree(old_fp); 
#endif
	//f (rcu_dereference(ibp->qp[0]))]))
	//list_del_rcu(&user->link);)
	//list_add_rcu(&new_user->link, &intf->users);)
	if (sync)
		synchronize_rcu();
}

static unsigned int flags;
static void rcuhood_read_unlock(void)
{
	//printk("zz %s %d \n", __func__, __LINE__);

	spin_unlock_irqrestore(&hook_rcu_spinlock, flags);
	//local_irq_enable();
	//rcu_read_unlock();
}

static void rcuhood_read_lock(void)
{
	void *retval;
	int i;
    //rcu_read_lock();
	spin_lock_irqsave(&hook_rcu_spinlock, flags);
	//while(1);
	//local_irq_disable();
    retval = rcu_dereference(gbl_foo);
	//printk("zz %s retval:%08x \n",__func__, (int)retval);
}

static long rcu_hook_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
		case 1:
			rcuhood_read_lock();
			break;

		case 2:
			rcuhood_read_unlock();
			break;

		case 3:
			rcu_write(1);
			break;

		case 4:
			rcu_write(0);
			break;

		default:
			ret = -EINVAL;
			goto OUT;
			break;
	}

OUT:
	return ret;
}

void rcu_hook_int(void)
{
	RCU_INIT_POINTER(rcu_pointer, NULL);
	spin_lock_init(&hook_rcu_spinlock);
	gbl_foo = kmalloc(1024, GFP_KERNEL); 
	printk("zz %s gbl_foo:%08x \n",__func__, (int)gbl_foo);
	if (misc_register(&rcu_hook_dev)) {
		pr_err("misc register err\n");
	}
}

void rcu_hook_exit(void)
{
	misc_deregister(&rcu_hook_dev);
}

