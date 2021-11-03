#ifndef __COMMON_HEAD_H__
#define __COMMON_HEAD_H__


void ioctl_data_init(struct ioctl_data *data);
int ruc_test(int fd);
int block_usage(int argc, char **argv);
int kmem_usage(int argc, char **argv);
int ktime_usage(int argc, char **argv);
int rcu_usage(int argc, char **argv);
int kprobe_usage(int argc, char **argv);
int lock_usage(int argc, char **argv);
int workqueue_usage(int argc, char **argv);
int radixtree_usage(int argc, char **argv);
int ksched_usage(int argc, char **argv);
int pci_usage(int argc, char **argv);
int panic_usage(int argc, char **argv);

#endif

