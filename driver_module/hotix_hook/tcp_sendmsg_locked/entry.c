#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/cpuset.h>
#include <linux/slab.h>
#include <linux/time64.h>
#include <linux/kprobes.h>
#include <linux/rwlock.h>
#include <linux/pid_namespace.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/syscalls.h>
#include <linux/tcp.h>
#include <net/tcp.h>
#include <linux/timekeeper_internal.h>
#include "../hotfix_util.h"

struct mutex *orig_text_mutex;
struct timekeeper *orig_timekeeper;

DEFINE_ORIG_FUNC(int, tcp_sendmsg_locked, 3, struct sock *, sk, struct msghdr *, msg, size_t , size);
TEXT_DECLARE()

unsigned long (*cust_kallsyms_lookup_name)(const char *name);
int (*old_tcp_sendmsg_locked)(struct sock *sk, struct msghdr *msg, size_t size);

static int (*orig_tcp_send_mss)(struct sock *sk, int *size_goal, int flags);
static void (*orig_skb_entail)(struct sock *sk, struct sk_buff *skb);
static void (*orig_tcp_push)(struct sock *sk, int flags, int mss_now,
		int nonagle, int size_goal);
static void (*orig_tcp_tx_timestamp)(struct sock *sk, u16 tsflags);
static void orig_tcp_remove_empty_skb.part.36(struct sock *sk, struct sk_buff *skb);


static inline bool forced_push(const struct tcp_sock *tp)
{
    return after(tp->write_seq, tp->pushed_seq + (tp->max_window >> 1));
}

static inline void tcp_mark_push(struct tcp_sock *tp, struct sk_buff *skb)
{
    TCP_SKB_CB(skb)->tcp_flags |= TCPHDR_PSH;
    tp->pushed_seq = tp->write_seq;
}

static int linear_payload_sz(bool first_skb)
{

    if (first_skb)
        return SKB_WITH_OVERHEAD(2048 - MAX_TCP_HEADER);
    return 0;
}

static int select_size(bool first_skb, bool zc)
{
    if (zc)
        return 0;
    return linear_payload_sz(first_skb);
}

int rewirte_tcp_sendmsg_locked(struct sock *sk, struct msghdr *msg, size_t size)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct ubuf_info *uarg = NULL;
	struct sk_buff *skb;
	struct sockcm_cookie sockc;
	int flags, err, copied = 0;
	int mss_now = 0, size_goal, copied_syn = 0;
	bool process_backlog = false;
	bool zc = false;
	long timeo;

	flags = msg->msg_flags;

	if (flags & MSG_ZEROCOPY && size && sock_flag(sk, SOCK_ZEROCOPY)) {
		if ((1 << sk->sk_state) & ~(TCPF_ESTABLISHED | TCPF_CLOSE_WAIT)) {
			err = -EINVAL;
			goto out_err;
		}

		skb = tcp_write_queue_tail(sk);
		uarg = sock_zerocopy_realloc(sk, size, skb_zcopy(skb));
		if (!uarg) {
			err = -ENOBUFS;
			goto out_err;
		}

		zc = sk->sk_route_caps & NETIF_F_SG;
		if (!zc)
			uarg->zerocopy = 0;
	}

#if 0
	if (unlikely(flags & MSG_FASTOPEN || inet_sk(sk)->defer_connect) &&
	    !tp->repair) {
		err = tcp_sendmsg_fastopen(sk, msg, &copied_syn, size);
		if (err == -EINPROGRESS && copied_syn > 0)
			goto out;
		else if (err)
			goto out_err;
	}
#endif

	timeo = sock_sndtimeo(sk, flags & MSG_DONTWAIT);

	tcp_rate_check_app_limited(sk);  /* is sending application-limited? */

	/* Wait for a connection to finish. One exception is TCP Fast Open
	 * (passive side) where data is allowed to be sent before a connection
	 * is fully established.
	 */
	if (((1 << sk->sk_state) & ~(TCPF_ESTABLISHED | TCPF_CLOSE_WAIT)) &&
	    !tcp_passive_fastopen(sk)) {
		err = sk_stream_wait_connect(sk, &timeo);
		if (err != 0)
			goto do_error;
	}

	if (unlikely(tp->repair)) {
		if (tp->repair_queue == TCP_RECV_QUEUE) {
			copied = tcp_send_rcvq(sk, msg, size);
			goto out_nopush;
		}

		err = -EINVAL;
		if (tp->repair_queue == TCP_NO_QUEUE)
			goto out_err;

		/* 'common' sending to sendq */
	}

	sockcm_init(&sockc, sk);
	if (msg->msg_controllen) {
		err = sock_cmsg_send(sk, msg, &sockc);
		if (unlikely(err)) {
			err = -EINVAL;
			goto out_err;
		}
	}

	/* This should be in poll */
	sk_clear_bit(SOCKWQ_ASYNC_NOSPACE, sk);

	/* Ok commence sending. */
	copied = 0;

restart:
	mss_now = orig_tcp_send_mss(sk, &size_goal, flags);

	err = -EPIPE;
	if (sk->sk_err || (sk->sk_shutdown & SEND_SHUTDOWN))
		goto do_error;

	while (msg_data_left(msg)) {
		int copy = 0;

		skb = tcp_write_queue_tail(sk);
		if (skb)
			copy = size_goal - skb->len;

		if (copy <= 0 || !tcp_skb_can_collapse_to(skb)) {
			bool first_skb;
			int linear;

new_segment:
			if (!sk_stream_memory_free(sk))
				goto wait_for_sndbuf;

			if (process_backlog && sk_flush_backlog(sk)) {
				process_backlog = false;
				goto restart;
			}
			first_skb = tcp_rtx_and_write_queues_empty(sk);
			linear = select_size(first_skb, zc);
			skb = sk_stream_alloc_skb(sk, linear, sk->sk_allocation,
						  first_skb);
			if (!skb)
				goto wait_for_memory;

			process_backlog = true;
			skb->ip_summed = CHECKSUM_PARTIAL;

			orig_skb_entail(sk, skb);
			copy = size_goal;

			/* All packets are restored as if they have
			 * already been sent. skb_mstamp isn't set to
			 * avoid wrong rtt estimation.
			 */
			if (tp->repair)
				TCP_SKB_CB(skb)->sacked |= TCPCB_REPAIRED;
		}

		/* Try to append data to the end of skb. */
		if (copy > msg_data_left(msg))
			copy = msg_data_left(msg);

		/* Where to copy to? */
		if (skb_availroom(skb) > 0 && !zc) {
			/* We have some space in skb head. Superb! */
			copy = min_t(int, copy, skb_availroom(skb));
			err = skb_add_data_nocache(sk, skb, &msg->msg_iter, copy);
			if (err)
				goto do_fault;
		} else if (!zc) {
			bool merge = true;
			int i = skb_shinfo(skb)->nr_frags;
			struct page_frag *pfrag = sk_page_frag(sk);

			if (!sk_page_frag_refill(sk, pfrag))
				goto wait_for_memory;

			if (!skb_can_coalesce(skb, i, pfrag->page,
					      pfrag->offset)) {
				if (i >= sysctl_max_skb_frags) {
					tcp_mark_push(tp, skb);
					goto new_segment;
				}
				merge = false;
			}

			copy = min_t(int, copy, pfrag->size - pfrag->offset);

			if (!sk_wmem_schedule(sk, copy))
				goto wait_for_memory;

			err = skb_copy_to_page_nocache(sk, &msg->msg_iter, skb,
						       pfrag->page,
						       pfrag->offset,
						       copy);
			if (err)
				goto do_error;

			/* Update the skb. */
			if (merge) {
				skb_frag_size_add(&skb_shinfo(skb)->frags[i - 1], copy);
			} else {
				skb_fill_page_desc(skb, i, pfrag->page,
						   pfrag->offset, copy);
				page_ref_inc(pfrag->page);
			}
			pfrag->offset += copy;
		} else {
			err = skb_zerocopy_iter_stream(sk, skb, msg, copy, uarg);
			if (err == -EMSGSIZE || err == -EEXIST) {
				tcp_mark_push(tp, skb);
				goto new_segment;
			}
			if (err < 0)
				goto do_error;
			copy = err;
		}

		if (!copied)
			TCP_SKB_CB(skb)->tcp_flags &= ~TCPHDR_PSH;

		tp->write_seq += copy;
		TCP_SKB_CB(skb)->end_seq += copy;
		tcp_skb_pcount_set(skb, 0);

		copied += copy;
		if (!msg_data_left(msg)) {
			if (unlikely(flags & MSG_EOR))
				TCP_SKB_CB(skb)->eor = 1;
			goto out;
		}

		if (skb->len < size_goal || (flags & MSG_OOB) || unlikely(tp->repair))
			continue;

		if (forced_push(tp)) {
			tcp_mark_push(tp, skb);
			__tcp_push_pending_frames(sk, mss_now, TCP_NAGLE_PUSH);
		} else if (skb == tcp_send_head(sk))
			tcp_push_one(sk, mss_now);
		continue;

wait_for_sndbuf:
		set_bit(SOCK_NOSPACE, &sk->sk_socket->flags);
wait_for_memory:
		if (copied)
			orig_tcp_push(sk, flags & ~MSG_MORE, mss_now,
				 TCP_NAGLE_PUSH, size_goal);

		err = sk_stream_wait_memory(sk, &timeo);
		if (err != 0)
			goto do_error;

		mss_now = orig_tcp_send_mss(sk, &size_goal, flags);
	}

out:
	if (copied) {
		orig_tcp_tx_timestamp(sk, sockc.tsflags);
		orig_tcp_push(sk, flags, mss_now, tp->nonagle, size_goal);
	}
out_nopush:
	sock_zerocopy_put(uarg);
	return copied + copied_syn;

do_error:
	skb = tcp_write_queue_tail(sk);
do_fault:
	orig_tcp_remove_empty_skb..part.36(sk, skb);

	if (copied + copied_syn)
		goto out;
out_err:
	sock_zerocopy_put_abort(uarg);
	err = sk_stream_error(sk, flags, err);
	/* make sure we wake any epoll edge trigger waiter */
	if (unlikely(skb_queue_len(&sk->sk_write_queue) == 0 &&
		     err == -EAGAIN)) {
		sk->sk_write_space(sk);
		tcp_chrono_stop(sk, TCP_CHRONO_SNDBUF_LIMITED);
	}
	return err;
}
EXPORT_SYMBOL_GPL(tcp_sendmsg_locked);
int new_tcp_sendmsg_locked(struct sock *sk, struct msghdr *msg, size_t size)
{
	int ret;
	ret = old_tcp_sendmsg_locked(sk, msg, size);
	return ret;
}

static int noop_pre_handler(struct kprobe *p, struct pt_regs *regs) { return 0; }

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

static int init_syms(void)
{
	if (get_kallsyms_lookup_name()) {
		printk("get_kallsyms_lookup_name failed\n");
		return -EINVAL;
	}

	TEXT_SYMS()
	LOOKUP_SYMS(tcp_sendmsg_locked);
	LOOKUP_SYMS(tcp_send_mss);
	LOOKUP_SYMS(skb_entail);
	LOOKUP_SYMS(tcp_push);
	LOOKUP_SYMS(tcp_tx_timestamp);
	return 0;
}

static int __init cpuset_trick_init(void)
{
	int ret = 0;

	if (init_syms())
		return -EINVAL;

	JUMP_INIT(tcp_sendmsg_locked);

	get_online_cpus();
	mutex_lock(orig_text_mutex);
	JUMP_INSTALLWITHOLD(tcp_sendmsg_locked);
	mutex_unlock(orig_text_mutex);
	put_online_cpus();

	return ret;
}

static void __exit cpuset_trick_exit(void)
{
	get_online_cpus();
	mutex_lock(orig_text_mutex);
	JUMP_REMOVE(tcp_sendmsg_locked);
	mutex_unlock(orig_text_mutex);
	put_online_cpus();
}

module_init(cpuset_trick_init);
module_exit(cpuset_trick_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");

