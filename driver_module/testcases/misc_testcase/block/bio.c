#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/of.h>
#include <linux/kthread.h>
#include <linux/reboot.h>
#include <linux/uaccess.h>
#include <linux/notifier.h>
#include <linux/interrupt.h>
#include <linux/rcupdate.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/blkdev.h>

#include <asm/stacktrace.h>
#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"
#include "medelay.h"
#include "mekernel.h"

void bio_dump_data(struct bio *bio)
{
	//struct bvec_iter iter;
	struct bio_vec      *bi_vecs;
	int i;
	bi_vecs = bio->bi_io_vec;

	printk("bi_phys_segments:%llx bi_vcnt:%lld bi_max_vecs:%lld bi_size:%llx bi_idx:%lld bi_done:%llx bi_bvec_dong:%llx op:%llx\n"
#if LINUX_VERSION_CODE <  KERNEL_VERSION(5,0,0)
			, (u64)bio->bi_phys_segments
#else
			,0
#endif
			, (u64)bio->bi_vcnt
			, (u64)bio->bi_max_vecs
			, (u64)bio->bi_iter.bi_size
			, (u64)bio->bi_iter.bi_idx
#if LINUX_VERSION_CODE <  KERNEL_VERSION(5,0,0)
			, (u64)bio->bi_iter.bi_done
#else
			,0
#endif
			, (u64)bio->bi_iter.bi_bvec_done
			, (u64)bio_op(bio)
		 	);

	for (i = 0; i <= bio->bi_vcnt; ++i) {
		printk("bv_page:%lx bv_len:%lx bv_offset:%lx \n", (unsigned long)bi_vecs->bv_page, (unsigned long)bi_vecs->bv_len, (unsigned long)bi_vecs->bv_offset);
		bi_vecs++;
	}
#if 0
	if (bi_vecs)
		printk("bv_page:%lx bv_len:%lx bv_offset:%lx \n", (unsigned long)bi_vecs->bv_page, (unsigned long)bi_vecs->bv_len, (unsigned long)bi_vecs->bv_offset);
#endif
}

