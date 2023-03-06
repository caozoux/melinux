#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/stddef.h>

#define KSYS_TRACE_BUF_SIZE 1024

struct ksys_trace_buffer {
	struct {
		char *data;
		unsigned int pos;
		int circle;
		unsigned int tail;
		spinlock_t lock;
		struct mutex mutex;
	} buffer;

	struct {
		char *data;
		unsigned int len;
	} product;

	char fmt_buffer[KSYS_TRACE_BUF_SIZE];
	unsigned int buf_size;
};

int init_ksys_trace_buffer(struct ksys_trace_buffer *buffer,
	unsigned int buf_size);
void destroy_ksys_trace_buffer(struct ksys_trace_buffer *buffer);
void discard_ksys_trace_buffer(struct ksys_trace_buffer *buffer);
void backup_ksys_trace_buffer(struct ksys_trace_buffer *buffer);
asmlinkage int
ksys_trace_buffer_printk_nolock(struct ksys_trace_buffer *buffer,
	const char *fmt, ...);
asmlinkage int
ksys_trace_buffer_printk(struct ksys_trace_buffer *buffer,
	const char *fmt, ...);
asmlinkage int
ksys_trace_buffer_write_nolock(struct ksys_trace_buffer *buffer,
	const void *data, size_t len);
asmlinkage int
ksys_trace_buffer_write(struct ksys_trace_buffer *buffer,
	const void *data, size_t len);
#define ksys_trace_buffer_spin_lock(__buffer, flags)	\
	spin_lock_irqsave(&((__buffer)->buffer.lock), flags)
#define ksys_trace_buffer_spin_unlock(__buffer, flags)	\
	spin_unlock_irqrestore(&((__buffer)->buffer.lock), flags)
void ksys_trace_buffer_mutex_lock(struct ksys_trace_buffer *buffer);
void ksys_trace_buffer_mutex_unlock(struct ksys_trace_buffer *buffer);

void ksysnose_trace_buffer_stack_trace(int pre, struct ksys_trace_buffer *buffer,
	struct task_struct *p, unsigned long *backtrace);
void ksysnose_trace_buffer_nolock_stack_trace(int pre, struct ksys_trace_buffer *buffer,
	struct task_struct *p, unsigned long *backtrace);
void ksysnose_trace_buffer_nolock_stack_trace_user(int pre, struct ksys_trace_buffer *buffer,
	unsigned long *backtrace);
void ksysnose_trace_buffer_stack_trace_user(int pre, struct ksys_trace_buffer *buffer,
	unsigned long *backtrace);
void ksysnose_trace_buffer_nolock_stack_trace_user_tsk(int pre, struct ksys_trace_buffer *buffer,
	struct task_struct *tsk, unsigned long *backtrace);
void ksysnose_trace_buffer_stack_trace_user_tsk(int pre, struct ksys_trace_buffer *buffer,
	struct task_struct *tsk, unsigned long *backtrace);
void ksys_trace_buffer_process_chain(int pre, struct ksys_trace_buffer *buffer, struct task_struct *tsk);
void ksys_trace_buffer_nolock_process_chain(int pre, struct ksys_trace_buffer *buffer, struct task_struct *tsk);
void ksys_trace_buffer_process_chain_cmdline(int pre, struct ksys_trace_buffer *buffer, struct task_struct *tsk);
void ksys_trace_buffer_nolock_process_chain_cmdline(int pre, struct ksys_trace_buffer *buffer, struct task_struct *tsk);
void trace_buffer_cgroups_tsk(int pre, struct ksys_trace_buffer *buffer, struct task_struct *tsk);
void trace_buffer_nolock_cgroups_tsk(int pre, struct ksys_trace_buffer *buffer, struct task_struct *tsk);
void trace_buffer_cgroups(int pre, struct ksys_trace_buffer *buffer);
void trace_buffer_nolock_cgroups(int pre, struct ksys_trace_buffer *buffer);
void ksys_trace_buffer_all_task_stack(int pre, struct ksys_trace_buffer *buffer);
void ksys_trace_buffer_nolock_all_task_stack(int pre,
	struct ksys_trace_buffer *buffer);
void ksysnose_trace_buffer_nolock_stack_trace_unfold(int pre, struct ksys_trace_buffer *buffer,
	struct task_struct *p, unsigned long *backtrace);
void ksysnose_trace_buffer_nolock_stack_trace_unfold_user(int pre, struct ksys_trace_buffer *buffer,
	unsigned long *backtrace);
void ksysnose_print_stack_trace_unfold_user_tsk(int pre, int might_sleep, struct task_struct *tsk, unsigned long *backtrace);
void ksysnose_trace_buffer_nolock_stack_trace_unfold_user_tsk(int pre, int might_sleep, struct ksys_trace_buffer *buffer,
	struct task_struct *tsk, unsigned long *backtrace);
void ksysnose_trace_buffer_stack_trace_unfold_user_tsk(int pre, int might_sleep, struct ksys_trace_buffer *buffer,
	struct task_struct *tsk, unsigned long *backtrace);
