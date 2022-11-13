#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <ksioctl/kinject_ioctl.h>

#include "ksysdata.h"
#include "ksysd_ioctl.h"

struct static_key inject_statickey_data;

int kinject_test_statickey(int enable)
{

	if (static_key_false(&inject_statickey_data)) {
		pr_info("inject_statickey_data false\n");
	} else {
		pr_info("inject_statickey_data true\n");
	}

	return 0;
}

