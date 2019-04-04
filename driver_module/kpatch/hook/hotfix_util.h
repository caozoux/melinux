#ifndef HOTFIX_UTIL_H
#define HOTFIX_UTIL_H

#include <linux/version.h>

#define LOOKUP_SYMS(name) do {							\
		orig_##name = (void *)kallsyms_lookup_name(#name);		\
		if (!orig_##name) {						\
			pr_err("kallsyms_lookup_name: %s\n", #name);		\
			return -EINVAL;						\
		}								\
	} while (0)

#define RELATIVEJUMP_SIZE   5

#define JUMP_INIT(func) do {							\
		e9_##func[0] = 0xe9;						\
		(*(int *)(&e9_##func[1])) = (long)new_##func -			\
		(long) orig_##func - RELATIVEJUMP_SIZE;				\
	} while (0)

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,12,0)
	#define JUMP_INSTALL(func) do {						\
			memcpy(inst_##func, orig_##func, RELATIVEJUMP_SIZE);	\
			orig_text_poke_smp(orig_##func, e9_##func,		\
					   RELATIVEJUMP_SIZE);			\
		} while (0)

	#define JUMP_REMOVE(func)						\
		orig_text_poke_smp(orig_##func, inst_##func, RELATIVEJUMP_SIZE)
#else
	#define JUMP_INSTALL(func) do {						\
			memcpy(inst_##func, orig_##func, RELATIVEJUMP_SIZE);	\
			orig_text_poke_bp(orig_##func, e9_##func,		\
					   RELATIVEJUMP_SIZE, new_##func);	\
		} while (0)

	#define JUMP_REMOVE(func)						\
		orig_text_poke_bp(orig_##func, inst_##func,			\
					RELATIVEJUMP_SIZE, new_##func)
#endif

#define DEFINE_ORIG_FUNC(rt, name, x, ...)					\
	unsigned char e9_##name[RELATIVEJUMP_SIZE];				\
	unsigned char inst_##name[RELATIVEJUMP_SIZE];				\
	extern rt new_##name(__MAP(x, __SC_DECL, __VA_ARGS__));			\
	rt (*orig_##name)(__MAP(x, __SC_DECL, __VA_ARGS__))

#endif
