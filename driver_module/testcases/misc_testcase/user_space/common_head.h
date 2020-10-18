#ifndef __COMMON_HEAD_H__
#define __COMMON_HEAD_H__


int ruc_test(int fd);
int block_usage(int argc, char **argv);
int kmem_usage(int argc, char **argv);
int ktime_usage(int argc, char **argv);
int rcu_usage(int argc, char **argv);
int kprobe_usage(int argc, char **argv);
int lock_usage(int argc, char **argv);
int workqueue_usage(int argc, char **argv);

#endif

