#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/netdevice.h>
#include <net/ip_fib.h>
#include <ksioctl/knet_ioctl.h>

#include "hotfix_util.h"
#include "ksysdata.h"
#include "ksysd_ioctl.h"
#include "internal.h"

static int fib_lookup_cp(struct net *net, const struct flowi4 *flp,
                 struct fib_result *res, unsigned int flags)
{
    struct fib_table *tb;
    int err = -ENETUNREACH;

    rcu_read_lock();

    tb = fib_get_table(net, RT_TABLE_MAIN);
    if (tb)
        err = fib_table_lookup(tb, flp, res, flags | FIB_LOOKUP_NOREF);

    if (err == -EAGAIN)
        err = -ENETUNREACH;

    rcu_read_unlock();

    return err;
}

static int netdev_scan_itera_func(struct net *net, struct net_device *dev)
{
	struct flowi4 fl4;
	struct fib_result res;
	int ret ;

	//printk("zz %s net:%lx \n",__func__, (unsigned long)net);
	ret = fib_lookup(net, &fl4, &res, 0);
	printk("zz %s ret:%lx \n",__func__, (unsigned long)ret);

	return 0;
}

int knet_unit_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_ksdata *ksdata)
{
	struct knet_ioctl kioctl;
	int ret;

	if (copy_from_user(&kioctl, (char __user *)ksdata->data, ksdata->len)) {
		pr_err("ioctl data copy err\n");
		ret = -EFAULT;
		goto OUT;
	}

	switch (ksdata->subcmd) {
		default:
			break;
	}

	return 0;
OUT:
	return ret;
}

int knet_unit_sym_init(void)	
{
	return 0;
}

int knet_unit_init(void)
{
	if (knet_unit_sym_init())	
		return -EINVAL;

	kdevice_scan_net_device(netdev_scan_itera_func);
	//knet_scan_net_device();
	return 0;
}

int knet_unit_exit(void)
{
	return 0;
}

