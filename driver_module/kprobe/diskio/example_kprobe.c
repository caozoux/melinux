#include <linux/udp.h>
#include <linux/kprobes.h>
#include <linux/bio.h>
#include <linux/genhd.h>
#include <linux/netdevice.h>
#include <linux/syscore_ops.h>

u64 stack_dump = 0;
static int kprobe_bio_add_page(struct kprobe *kp, struct pt_regs *regs)
{
    struct bio *bio = (struct bio *)regs->di;
    if (bio && bio->bi_disk) {
         struct gendisk *bi_disk = bio->bi_disk;
         if (strstr(bi_disk->disk_name, "nbd")) {
           if (stack_dump++ <10)   
            dump_stack();
        }
        trace_printk("zz %s %d %s\n", __func__, __LINE__, bi_disk->disk_name);
    }

    return 0;
}

static int kprobe_generic_make_request_checks(struct kprobe *kp, struct pt_regs *regs)
{
    struct bio *bio = (struct bio *)regs->di;
    if (bio && bio->bi_disk) {
         struct gendisk *bi_disk = bio->bi_disk;
         if (strstr(bi_disk->disk_name, "nbd")) {
           if (stack_dump++ <10)   
            dump_stack();
        }
        trace_printk("zz %s %d %s\n", __func__, __LINE__, bi_disk->disk_name);
    }

    return 0;
}

static int kprobe_vfs_write(struct kprobe *kp, struct pt_regs *regs)
{
    struct file *file = (struct file*)regs->di;
    if (strcmp("qemu-system-x86_64", current->comm))
    {
       if (file && file->f_path.dentry) {
          if (file->f_path.dentry->d_iname) 
            trace_printk("filename:%s\n", file->f_path.dentry->d_iname);
          if (file->f_inode && file->f_inode->i_sb)
            trace_printk("filename:%s\n", file->f_inode->i_sb->s_id);
        } 
    }
    return 0;
}

struct kprobe kplist[] = {
#if 0
    {
        .symbol_name = "bio_add_page",
        .pre_handler = kprobe_bio_add_page,
    },
#endif
    {
        .symbol_name = "vfs_write",
        .pre_handler = kprobe_vfs_write,
    },
    {
        .symbol_name = "generic_make_request_checks",
        .pre_handler = kprobe_generic_make_request_checks,
    },
};

static int __init kprobe_driver_init(void)
{
    int i;
    for (i = 0; i < sizeof(kplist)/sizeof(struct kprobe); ++i) {
        if (register_kprobe(&kplist[i])) {
            printk("register kprobe failed:%s \n", kplist[i].symbol_name);
            while (i>0) {
                i--;
                unregister_kprobe(&kplist[i]);
            }
            goto out;
        }
    }
    printk("zz %s %d \n", __func__, __LINE__);
    return 0;
out:
    return -EINVAL;
}

static void __exit kprobe_driver_exit(void)
{
    int i;
    for (i = 0; i < sizeof(kplist)/sizeof(struct kprobe); ++i) {
        unregister_kprobe(&kplist[i]);
    }
    printk("zz %s %d \n", __func__, __LINE__);
}

module_init(kprobe_driver_init);
module_exit(kprobe_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
