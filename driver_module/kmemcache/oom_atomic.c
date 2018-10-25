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
#include "kmemcache.h"


void oom_atomic_test()
{
	void *addr;		
	while(1) {
		addr = kmalloc(1024*1024*4, GFP_ATOMIC);
	}
		
}
