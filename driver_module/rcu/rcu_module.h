#ifndef __SLUB_MODULE_H__
#define __SLUB_MODULE_H__


int __maybe_unused kmemcache_flag_rcu_init(void);
int __maybe_unused kmemcache_flag_rcu_exit(void);
void rcu_hook_int(void);
void rcu_hook_exit(void);
int __maybe_unused kmemcache_flag_rcu_init(void);
int __maybe_unused kmemcache_flag_rcu_exit(void);
#endif

