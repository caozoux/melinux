#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <hotfix_util.h>

#include "local.h"
#include "internal.h"

struct list_head *orig_net_namespace_list;

int kdevice_scan_net_device(netdev_scan_itera func)
{
	struct net *net;
	struct net_device *dev;

	list_for_each_entry(net, orig_net_namespace_list, list) {
		for_each_netdev(net, dev)
			func(net,dev);
	}

	return 0;
}
