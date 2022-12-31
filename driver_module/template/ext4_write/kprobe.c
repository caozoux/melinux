#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/ip.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/kprobes.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/skbuff.h>
#include <linux/udp.h>
#include <linux/netdevice.h>
#include <linux/syscore_ops.h>

static int kprobe_ext4_file_write_iter(struct kprobe *kp, struct pt_regs *regs)
{
  struct kiocb *iocb = (void *) regs->di;
  //struct file *file = iocb->ki_filp;
  //struct dentry *f= file->f_path.dentry;


  return 0;
}

struct kprobe kplist[] = {
  {
        .symbol_name = "ext4_file_write_iter",
        .pre_handler = kprobe_ext4_file_write_iter,
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

