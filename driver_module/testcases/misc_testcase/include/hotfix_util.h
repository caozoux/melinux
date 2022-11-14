#ifndef HOTFIX_UTIL_H
#define HOTFIX_UTIL_H

#include <linux/version.h>
#include <linux/syscalls.h>

#if 0
#define LOOKUP_SYMS(name) do {							\
		orig_##name = (void *)kallsyms_lookup_name(#name);		\
		if (!orig_##name) {						\
			pr_err("kallsyms_lookup_name: %s\n", #name);		\
			return -EINVAL;						\
		}								\
	} while (0)
#else
extern unsigned long (*cust_kallsyms_lookup_name)(const char *name);
#define LOOKUP_SYMS(name) do {							\
		orig_##name = (void *)cust_kallsyms_lookup_name(#name);		\
		if (!orig_##name) {						\
			pr_err("kallsyms_lookup_name: %s\n", #name);		\
			return -EINVAL;						\
		}								\
	} while (0)
#endif

#define RELATIVEJUMP_SIZE   5

#ifdef CONFIG_X86
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
	#define JUMP_INSTALLWITHOLD(func) do {						\
			memcpy(inst_##func, orig_##func, RELATIVEJUMP_SIZE);	\
	        old_##func = orig_##func + 5;                   \
			orig_text_poke_smp(orig_##func, e9_##func,		\
					   RELATIVEJUMP_SIZE);			\
		} while (0)

	#define JUMP_REMOVE(func)						\
		orig_text_poke_smp(orig_##func, inst_##func, RELATIVEJUMP_SIZE)
    #define TEXT_DECLARE()  void *(*orig_text_poke_smp)(void *addr,  \
			          const void *opcode, size_t len);
    #define TEXT_SYMS()     LOOKUP_SYMS(text_poke_smp); \
		                    LOOKUP_SYMS(text_mutex);
			          
#else
	#define JUMP_INSTALL(func) do {						\
			memcpy(inst_##func, orig_##func, RELATIVEJUMP_SIZE);	\
			orig_text_poke_bp(orig_##func, e9_##func,		\
					   RELATIVEJUMP_SIZE, new_##func);	\
		} while (0)

	#define JUMP_INSTALLWITHOLD(func) do {						\
			memcpy(inst_##func, orig_##func, RELATIVEJUMP_SIZE);	\
	        old_##func = orig_##func + 5;                   \
			orig_text_poke_bp(orig_##func, e9_##func,		\
					   RELATIVEJUMP_SIZE, new_##func);	\
		} while (0)

	#define JUMP_REMOVE(func)						\
		orig_text_poke_bp(orig_##func, inst_##func,			\
					RELATIVEJUMP_SIZE, new_##func)
    #define TEXT_DECLARE()  void * (*orig_text_poke_bp)(void *addr, \
						const void *opcode, size_t len, void *handler);
    #define TEXT_SYMS()     LOOKUP_SYMS(text_poke_bp); \
		                    LOOKUP_SYMS(text_mutex);
#endif

#else //arm64

#define JUMP_INIT(func) do {                                                \
		unsigned long long addr = (unsigned long long)&new_##func;      \
		/* stp x29, x30, [sp,#-16]! */              \
		e9_##func[0] = 0xa9bf7bfdu;                 \
		/* mov x29, #0x0 */                         \
		e9_##func[1] = 0xd280001du | ((addr & 0xffff) << 5);        \
		/* movk    x29, #0x0, lsl #16 */                \
		e9_##func[2] = 0xf2a0001du | (((addr & 0xffff0000) >> 16) << 5);        \
		/* movk    x29, #0x0, lsl #32 */                \
		e9_##func[3] = 0xf2c0001du | (((addr & 0xffff00000000) >> 32) << 5);    \
		/* movk    x29, #0x0, lsl #48 */                \
		e9_##func[4] = 0xf2e0001du | (((addr & 0xffff000000000000) >> 48) << 5);   \
		/* blr x29 */                                   \
		e9_##func[5] = 0xd63f03a0u;                     \
		/* ldp x29, x30, [sp],#16 */                    \
		e9_##func[6] = 0xa8c17bfdu;                     \
		/* ret */                                       \
		e9_##func[7] = 0xd65f03c0u;                     \
	} while (0)

#define JUMP_INSTALL(func) do {                     \
			memcpy(inst_##func, orig_##func, RELATIVEJUMP_SIZE);    \
			/* memcpy(orig_##func, e9_##func, RELATIVEJUMP_SIZE); */   \
			/* __flush_cache(orig_##func, RELATIVEJUMP_SIZE);   */  \
			orig_aarch64_insn_patch_text((void **)orig_##func, (u32 *)e9_##func, RELATIVEJUMP_SIZE);    \
		} while (0)

#define JUMP_REMOVE(func)                       \
            /* memcpy(orig_##func, inst_##func, RELATIVEJUMP_SIZE); */ \
            /* __flush_cache(orig_##func, RELATIVEJUMP_SIZE); */    \
            orig_aarch64_insn_patch_text((void **)orig_##func, (u32 *)inst_##func, RELATIVEJUMP_SIZE);
    #define TEXT_DECLARE()  int (*orig_aarch64_insn_patch_text)(void *addrs[], u32 insns[], int cnt);
    #define TEXT_SYMS()     LOOKUP_SYMS(aarch64_insn_patch_text);
		                    

#endif

#define DEFINE_ORIG_FUNC(rt, name, x, ...)					\
	unsigned char e9_##name[RELATIVEJUMP_SIZE];				\
	unsigned char inst_##name[RELATIVEJUMP_SIZE];				\
	extern rt new_##name(__MAP(x, __SC_DECL, __VA_ARGS__));			\
	rt (*orig_##name)(__MAP(x, __SC_DECL, __VA_ARGS__)); \
	rt (*old_##name)(__MAP(x, __SC_DECL, __VA_ARGS__)); \
	rt new_##name(__MAP(x, __SC_DECL, __VA_ARGS__))


#endif
