#ifndef __KBASE_H__
#define __KBASE_H__

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
#define PROC_MARCO(__name) \
    static const struct proc_ops __name ## _fops = { \
        .proc_open       = __name ## _open, \
        .proc_read       = seq_read,   \
        .proc_write      = __name ## _write, \
        .proc_lseek     = seq_lseek, \
        .proc_release    = single_release, \
    };
#else
#define PROC_MARCO(__name) \
    static const struct file_operations __name ## _fops = { \
        .open       = __name ## _open, \
        .read       = seq_read,   \
        .write      = __name ## _write, \
        .llseek     = seq_lseek, \
        .release    = single_release, \
    };
#endif

struct slabinfo {
    unsigned long active_objs;
    unsigned long num_objs;
    unsigned long active_slabs;
    unsigned long num_slabs;
    unsigned long shared_avail;
    unsigned int limit;
    unsigned int batchcount;
    unsigned int shared;
    unsigned int objects_per_slab;
    unsigned int cache_order;
};

struct task_struct *find_process_by_pid(pid_t vnr);
void get_slabinfo(struct kmem_cache *s, struct slabinfo *sinfo);

#endif /* ifndef __KBASE_H__ */
