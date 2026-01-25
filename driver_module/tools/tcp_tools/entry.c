#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kallsyms.h>
#include <linux/kprobes.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/livepatch.h>
#include <linux/tcp.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <net/sock.h>
#include <linux/inet.h>
#include <net/inet_sock.h>
#include "kernel/sched/sched.h"

#define LOOKUP_SYMS(name) do {							\
		orig_##name = (void *)cust_kallsyms_lookup_name(#name);		\
		if (!orig_##name) {						\
			pr_err("kallsyms_lookup_name: %s\n", #name);		\
			return -EINVAL;						\
		}								\
	} while (0)

#define LOOKUP_SYMSV1(name) do {							\
		orig_##name = (void *)cust_kallsyms_lookup_name(#name);		\
		if (!orig_##name) {						\
			pr_err("kallsyms_lookup_name: %s\n", #name);		\
			return -EINVAL;						\
		}								\
		old_##name = orig_##name + 5; \
	} while (0)

static char ksym_name[KSYM_NAME_LEN] = "pid_max";
module_param_string(ksym, ksym_name, KSYM_NAME_LEN, S_IRUGO);
MODULE_PARM_DESC(ksym, "Kernel symbol to monitor; this module will report any"
			" write operations on the kernel symbol");

unsigned long (*cust_kallsyms_lookup_name)(const char *name);
static int noop_pre_handler(struct kprobe *p, struct pt_regs *regs) { return 0; }

static struct proc_dir_entry *proc_root;
static struct proc_dir_entry *proc_tcp_tools;

/* TCP streak trace data structure */
struct tcp_streak_info {
	__be32 src_ip;
	__be32 dst_ip;
	__u16 src_port;
	__u16 dst_port;
	bool enabled;
};

static struct tcp_streak_info streak_trace = {
	.enabled = false,
};

/* Proc file read operation */
static int tcp_streak_trace_show(struct seq_file *m, void *v)
{
	if (streak_trace.enabled) {
		seq_printf(m, "%pI4:%u,%pI4:%u\n",
			&streak_trace.src_ip, streak_trace.src_port,
			&streak_trace.dst_ip, streak_trace.dst_port);
	} else {
		seq_printf(m, "disabled\n");
	}

	return 0;
}

/* Proc file open operation */
static int tcp_streak_trace_open(struct inode *inode, struct file *file)
{
	return single_open(file, tcp_streak_trace_show, NULL);
}

/* Parse IP address string */
static int parse_ip_port(const char *str, __be32 *ip, __u16 *port)
{
	char buf[64];
	char *colon;
	int ret;

	strncpy(buf, str, sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = '\0';

	colon = strchr(buf, ':');
	if (!colon)
		return -EINVAL;

	*colon = '\0';
	*port = htons(simple_strtoul(colon + 1, NULL, 10));

	ret = in4_pton(buf, -1, (u8 *)ip, -1, NULL);
	if (ret == 0)
		return -EINVAL;

	return 0;
}

/* Proc file write operation */
static ssize_t tcp_streak_trace_write(struct file *file, const char __user *buf,
		size_t count, loff_t *ppos)
{
	char kbuf[128];
	char *comma;
	int ret;
	__be32 src_ip, dst_ip;
	__u16 src_port, dst_port;

	if (count >= sizeof(kbuf))
		return -EINVAL;

	if (copy_from_user(kbuf, buf, count))
		return -EFAULT;

	kbuf[count] = '\0';

	/* Check if disabling */
	if (strncmp(kbuf, "disable", 7) == 0) {
		streak_trace.enabled = false;
		pr_info("TCP streak trace disabled\n");
		return count;
	}

	/* Parse format: src_ip:src_port,dst_ip:dst_port */
	comma = strchr(kbuf, ',');
	if (!comma) {
		pr_err("Invalid format, use: src_ip:src_port,dst_ip:dst_port\n");
		return -EINVAL;
	}

	*comma = '\0';

	ret = parse_ip_port(kbuf, &src_ip, &src_port);
	if (ret) {
		pr_err("Failed to parse src IP:port\n");
		return -EINVAL;
	}

	ret = parse_ip_port(comma + 1, &dst_ip, &dst_port);
	if (ret) {
		pr_err("Failed to parse dst IP:port\n");
		return -EINVAL;
	}

	streak_trace.src_ip = src_ip;
	streak_trace.dst_ip = dst_ip;
	streak_trace.src_port = src_port;
	streak_trace.dst_port = dst_port;
	streak_trace.enabled = true;

	pr_info("TCP streak trace set: %pI4:%u -> %pI4:%u\n",
		&src_ip, ntohs(src_port),
		&dst_ip, ntohs(dst_port));

	return count;
}

#if 0
static const struct proc_ops tcp_streak_trace_proc_ops = {
	.proc_open	= tcp_streak_trace_open,
	.proc_read	= seq_read,
	.proc_lseek	= seq_lseek,
	.proc_release	= single_release,
	.proc_write	= tcp_streak_trace_write,
};
#else
static const struct file_operations tcp_streak_trace_proc_ops = {
	.open	= tcp_streak_trace_open,
	.read	= seq_read,
	.llseek	= seq_lseek,
	.release	= single_release,
	.write	= tcp_streak_trace_write,
};
#endif

/* Dump tcp_sock memory allocation information */
static void dump_tcp_sock_memory_info(struct sock *sk)
{
	struct inet_sock *inet = inet_sk(sk);

	pr_info("=== TCP Socket Memory Info ===\n");
	pr_info("Connection: %pI4:%u -> %pI4:%u\n",
		&inet->inet_saddr, ntohs(inet->inet_sport),
		&inet->inet_daddr, ntohs(inet->inet_dport));
	pr_info("sk_rmem_alloc: %u (bytes allocated for receive buffer)\n",
		atomic_read(&sk->sk_rmem_alloc.counter));
	pr_info("sk_wmem_alloc: %u (bytes allocated for send buffer)\n",
		refcount_read(&sk->sk_wmem_alloc) * sizeof(struct sk_buff));
	pr_info("sk_sndbuf: %u (send buffer size)\n", sk->sk_sndbuf);
	pr_info("sk_rcvbuf: %u (receive buffer size)\n", sk->sk_rcvbuf);
	pr_info("============================\n");
}


static int (*orig_tcp_sendmsg_locked)(struct sock *sk, struct msghdr *msg, size_t size);
static int (*old_tcp_sendmsg_locked)(struct sock *sk, struct msghdr *msg, size_t size);
/*
 * Livepatch callback function for tcp_sendmsg_locked
 * This function will be called instead of the original tcp_sendmsg_locked
 */
static int livepatch_tcp_sendmsg_locked(struct sock *sk, struct msghdr *msg, size_t size)
{
	int ret;
	struct inet_sock *inet = inet_sk(sk);
	bool matched = false;

	/* Check if this connection matches the streak trace */
	if (streak_trace.enabled) {
		if (inet->inet_saddr == streak_trace.src_ip &&
		    inet->inet_daddr == streak_trace.dst_ip &&
		    inet->inet_sport == streak_trace.src_port &&
		    inet->inet_dport == streak_trace.dst_port) {
			matched = true;
		}
	}

	/* Print TCP connection info if matched */
	if (matched) {
		trace_printk("TCP MATCHED: src=%pI4:%u, dst=%pI4:%u, size=%zu\n",
			&inet->inet_saddr, ntohs(inet->inet_sport),
			&inet->inet_daddr, ntohs(inet->inet_dport),
			size);

		/* Dump memory allocation info */
		dump_tcp_sock_memory_info(sk);
	}

	ret = old_tcp_sendmsg_locked(sk, msg, size);

	return ret;
}

/* Livepatch function structure */
static struct klp_func funcs[] = {
	{
		.old_name = "tcp_sendmsg_locked",
		.new_func = livepatch_tcp_sendmsg_locked,
	}, {}
};

/* Livepatch object structure (vmlinux) */
static struct klp_object objs[] = {
	{
		.funcs = funcs,
	}, {}
};

/* Livepatch patch structure */
static struct klp_patch patch = {
	.mod = THIS_MODULE,
	.objs = objs,
};


static int get_kallsyms_lookup_name(void)
{
	int ret;
	struct kprobe kp;

	memset(&kp, 0, sizeof(struct kprobe));

	ret = -1;
	kp.symbol_name = "kallsyms_lookup_name";
	kp.pre_handler = noop_pre_handler;

	ret = register_kprobe(&kp);
	if (ret < 0) {
		printk("Err: find kallsyms_lookup_name failed \n");
		return -EINVAL;
	}

	cust_kallsyms_lookup_name = (void*)kp.addr;
	unregister_kprobe(&kp);

	return 0;
}

static int sym_init(void)
{
	if (get_kallsyms_lookup_name())
		return -EINVAL;

	LOOKUP_SYMSV1(tcp_sendmsg_locked);

	return 0;
}

static int __init hw_break_module_init(void)
{
	int ret;
	struct proc_dir_entry *entry;

	if (sym_init())
		return -EINVAL;

	/* Create /proc/tcp_tools directory */
	proc_tcp_tools = proc_mkdir("tcp_tools", NULL);
	if (!proc_tcp_tools) {
		pr_err("Failed to create /proc/tcp_tools\n");
		return -ENOMEM;
	}

	/* Create tcp_streak_trace file */
	entry = proc_create("tcp_streak_trace", 0666, proc_tcp_tools,
			&tcp_streak_trace_proc_ops);
	if (!entry) {
		pr_err("Failed to create /proc/tcp_tools/tcp_streak_trace\n");
		remove_proc_entry("tcp_tools", NULL);
		return -ENOMEM;
	}

	/* Register livepatch */
	ret = klp_enable_patch(&patch);
	if (ret) {
		pr_err("Failed to enable livepatch: %d\n", ret);
		remove_proc_entry("tcp_streak_trace", proc_tcp_tools);
		remove_proc_entry("tcp_tools", NULL);
		return ret;
	}

	pr_info("Livepatch enabled successfully\n");

	return 0;
}

static void __exit hw_break_module_exit(void)
{
	/* Disable livepatch */
	//klp_disable_patch(&patch);
	pr_info("Livepatch disabled\n");

	/* Remove proc entries */
	remove_proc_entry("tcp_streak_trace", proc_tcp_tools);
	remove_proc_entry("tcp_tools", NULL);
}

module_init(hw_break_module_init);
module_exit(hw_break_module_exit);

MODULE_LICENSE("GPL");
MODULE_INFO(livepatch, "Y");

