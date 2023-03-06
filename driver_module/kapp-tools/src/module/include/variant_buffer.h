#ifndef __KSYS_VARIANT_BUFFER
#define __KSYS_VARIANT_BUFFER

#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/stddef.h>

#include "uapi/ksys_config.h"

struct ksys_variant_buffer_head {
    unsigned long magic;
    unsigned long len;
};

struct ksys_variant_buffer {
	struct {
		char *data;
		unsigned int pos;
		struct ksys_variant_buffer_head *head;
	} buffers[2];
	/* 0 or 1 */
	int buffer_toggle;

	struct {
		char *data;
		unsigned int len;
	} product;

	unsigned int buf_size;
	unsigned int alloced;
	spinlock_t lock;
	struct mutex mutex;
};

#define ksys_variant_buffer_spin_lock(__buffer, flags)	\
	spin_lock_irqsave(&((__buffer)->lock), flags)
#define ksys_variant_buffer_spin_unlock(__buffer, flags)	\
	spin_unlock_irqrestore(&((__buffer)->lock), flags)
int init_ksys_variant_buffer(struct ksys_variant_buffer *buffer,
	unsigned int buf_size);
int alloc_ksys_variant_buffer(struct ksys_variant_buffer *buffer);
void destroy_ksys_variant_buffer(struct ksys_variant_buffer *buffer);
void discard_ksys_variant_buffer(struct ksys_variant_buffer *buffer);
void backup_ksys_variant_buffer(struct ksys_variant_buffer *buffer);
asmlinkage int
ksys_variant_buffer_reserve(struct ksys_variant_buffer *buffer, size_t len);
asmlinkage int
ksys_variant_buffer_seal(struct ksys_variant_buffer *buffer);
asmlinkage int
ksys_variant_buffer_write_nolock(struct ksys_variant_buffer *buffer,
	const void *data, size_t len);
void ksys_variant_buffer_mutex_lock(struct ksys_variant_buffer *buffer);
void ksys_variant_buffer_mutex_unlock(struct ksys_variant_buffer *buffer);
int copy_to_user_variant_buffer(struct ksys_variant_buffer *variant_buffer,
	void __user *ptr_len, void __user *buf, size_t size);
#endif /* __KSYS_VARIANT_BUFFER */
