#ifndef __MEDRIVER_KPROBE_H__
#define __MEDRIVER_KPROBE_H__

struct kprobe_item {
	int enable;
	char *symbol;
	kprobe_pre_handler_t pre_handler;
	struct kprobe kprobe;
};

static inline void medriver_unregister_kprobe(struct kprobe_item *kpro)
{
	if (kpro->enable)
    		unregister_kprobe(&kpro->kprobe);
}

static inline int medriver_register_kprobe(struct kprobe_item *kpro)
{
	if (kpro->enable) {
		kpro->kprobe.symbol_name= kpro->symbol;
		kpro->kprobe.pre_handler= kpro->pre_handler;
    		return register_kprobe(&kpro->kprobe);
	}
	return 0;
}
#endif
