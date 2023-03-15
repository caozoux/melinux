#ifndef __KBASE_H__
#define __KBASE_H__


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
