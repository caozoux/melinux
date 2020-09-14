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
#include <linux/blkdev.h>

#include <asm/stacktrace.h>
#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"
#include "medelay.h"
#include "mekernel.h"

static bio_vec(struct bio *bio)
{

}

void bio_dump_data(struct bio *bio)
{
	struct bvec_iter iter;
	struct bio_vec      *bi_vecs;
	bi_vecs = bio->bi_inline_vecs;

	printk("bi_phys_segments:%lx bi_vcnt:%d bi_max_vecs:%d bi_size:%x bi_idx:%d bi_done:%x bi_bvec_dong:%x"
			, bio->bi_phys_segments
			, bio->bi_vcnt
			, bio->bi_max_vecs
			, bio->bi_iter.bi_size
			, bio->bi_iter.bi_idx
			, bio->bi_iter.bi_done
			, bio->bi_iter.bi_bvec_done
		 	);

	if (bi_vecs)
		printk("bv_page:%lx bv_len:%lx bv_offset:%lx \n", (unsigned long)bi_vecs->bv_page, (unsigned long)bi_vecs->bv_len, (unsigned long)bi_vecs->bv_offset);
}
