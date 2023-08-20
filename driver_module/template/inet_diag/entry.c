#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/kprobes.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/smpboot.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/nmi.h>
#include <linux/inet_diag.h>
#include <linux/netlink.h>
#include <net/sock.h>
#include <net/inet_hashtables.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/kallsyms.h>
#include <linux/livepatch.h>

#define LOOKUP_SYMS(name) do {							\
			orig_##name = (void *)kallsyms_lookup_name(#name);		\
			if (!orig_##name) {						\
				pr_err("kallsyms_lookup_name: %s\n", #name);		\
				return -EINVAL;						\
			}								\
		} while (0)



static int (*orig_sk_diag_fill)(struct sock *sk, struct sk_buff *skb,
			const struct inet_diag_req_v2 *r,
			struct user_namespace *user_ns,
			u32 portid, u32 seq, u16 nlmsg_flags,
			const struct nlmsghdr *unlh, bool net_admin);
int (*orig_inet_diag_bc_sk)(const struct nlattr *bc, struct sock *sk);


static int inet_csk_diag_fill(struct sock *sk,
                  struct sk_buff *skb,
                  const struct inet_diag_req_v2 *req,
                  struct user_namespace *user_ns,
                  u32 portid, u32 seq, u16 nlmsg_flags,
                  const struct nlmsghdr *unlh,
                  bool net_admin)
{
    return inet_sk_diag_fill(sk, inet_csk(sk), skb, req, user_ns,
                 portid, seq, nlmsg_flags, unlh, net_admin);
}

static int inet_csk_diag_dump(struct sock *sk,
                  struct sk_buff *skb,
                  struct netlink_callback *cb,
                  const struct inet_diag_req_v2 *r,
                  const struct nlattr *bc,
                  bool net_admin)
{
    if (!orig_inet_diag_bc_sk(bc, sk))
        return 0;

    return inet_csk_diag_fill(sk, skb, r,
                  sk_user_ns(NETLINK_CB(cb->skb).sk),
                  NETLINK_CB(cb->skb).portid,
                  cb->nlh->nlmsg_seq, NLM_F_MULTI, cb->nlh,
                  net_admin);
}

static int (*orig_sk_diag_fill)(struct sock *sk, struct sk_buff *skb,
			const struct inet_diag_req_v2 *r,
			struct user_namespace *user_ns,
			u32 portid, u32 seq, u16 nlmsg_flags,
			const struct nlmsghdr *unlh, bool net_admin);

static void twsk_build_assert(void)
{
    BUILD_BUG_ON(offsetof(struct inet_timewait_sock, tw_family) !=
             offsetof(struct sock, sk_family));

    BUILD_BUG_ON(offsetof(struct inet_timewait_sock, tw_num) !=
             offsetof(struct inet_sock, inet_num));

    BUILD_BUG_ON(offsetof(struct inet_timewait_sock, tw_dport) !=
             offsetof(struct inet_sock, inet_dport));

    BUILD_BUG_ON(offsetof(struct inet_timewait_sock, tw_rcv_saddr) !=
             offsetof(struct inet_sock, inet_rcv_saddr));

    BUILD_BUG_ON(offsetof(struct inet_timewait_sock, tw_daddr) !=
             offsetof(struct inet_sock, inet_daddr));

#if IS_ENABLED(CONFIG_IPV6)
    BUILD_BUG_ON(offsetof(struct inet_timewait_sock, tw_v6_rcv_saddr) !=
             offsetof(struct sock, sk_v6_rcv_saddr));

    BUILD_BUG_ON(offsetof(struct inet_timewait_sock, tw_v6_daddr) !=
             offsetof(struct sock, sk_v6_daddr));
#endif
}

void new_inet_diag_dump_icsk(struct inet_hashinfo *hashinfo, struct sk_buff *skb,
								struct netlink_callback *cb,
								 const struct inet_diag_req_v2 *r, struct nlattr *bc)
{
    bool net_admin = netlink_net_capable(cb->skb, CAP_NET_ADMIN);
    struct net *net = sock_net(skb->sk);
    u32 idiag_states = r->idiag_states;
    int i, num, s_i, s_num;
    struct sock *sk;

    if (idiag_states & TCPF_SYN_RECV)
        idiag_states |= TCPF_NEW_SYN_RECV;
    s_i = cb->args[1];
    s_num = num = cb->args[2];

    if (cb->args[0] == 0) {
        if (!(idiag_states & TCPF_LISTEN) || r->id.idiag_dport)
            goto skip_listen_ht;

        for (i = s_i; i < INET_LHTABLE_SIZE; i++) {
            struct inet_listen_hashbucket *ilb;

            num = 0;
            ilb = &hashinfo->listening_hash[i];
            spin_lock(&ilb->lock);
            sk_for_each(sk, &ilb->head) {
                struct inet_sock *inet = inet_sk(sk);

                if (!net_eq(sock_net(sk), net))
                    continue;

                if (num < s_num) {
                    num++;
                    continue;
                }

                if (r->sdiag_family != AF_UNSPEC &&
                    sk->sk_family != r->sdiag_family)
                    goto next_listen;

                if (r->id.idiag_sport != inet->inet_sport &&
                    r->id.idiag_sport)
                    goto next_listen;

                if (inet_csk_diag_dump(sk, skb, cb, r,
                               bc, net_admin) < 0) {
                    spin_unlock(&ilb->lock);
                    goto done;
                }

next_listen:
                ++num;
            }
            spin_unlock(&ilb->lock);

            s_num = 0;
        }
skip_listen_ht:
        cb->args[0] = 1;
        s_i = num = s_num = 0;
    }

    if (!(idiag_states & ~TCPF_LISTEN))
        goto out;

#define SKARR_SZ 16
    for (i = s_i; i <= hashinfo->ehash_mask; i++) {
        struct inet_ehash_bucket *head = &hashinfo->ehash[i];
        spinlock_t *lock = inet_ehash_lockp(hashinfo, i);
        struct hlist_nulls_node *node;
        struct sock *sk_arr[SKARR_SZ];
        int num_arr[SKARR_SZ];
        int idx, accum, res;

        if (hlist_nulls_empty(&head->chain))
            continue;

        if (i > s_i)
            s_num = 0;

next_chunk:
        num = 0;
        accum = 0;
        spin_lock_bh(lock);
        sk_nulls_for_each(sk, node, &head->chain) {
            int state;

            if (!net_eq(sock_net(sk), net))
                continue;
            if (num < s_num)
                goto next_normal;
            state = (sk->sk_state == TCP_TIME_WAIT) ?
                inet_twsk(sk)->tw_substate : sk->sk_state;
            if (!(idiag_states & (1 << state)))
                goto next_normal;
            if (r->sdiag_family != AF_UNSPEC &&
                sk->sk_family != r->sdiag_family)
                goto next_normal;
            if (r->id.idiag_sport != htons(sk->sk_num) &&
                r->id.idiag_sport)
                goto next_normal;
            if (r->id.idiag_dport != sk->sk_dport &&
                r->id.idiag_dport)
                goto next_normal;
            twsk_build_assert();

            if (!orig_inet_diag_bc_sk(bc, sk))
                goto next_normal;

            sock_hold(sk);
            num_arr[accum] = num;
            sk_arr[accum] = sk;
            if (++accum == SKARR_SZ)
                break;
next_normal:
            ++num;
        }
        spin_unlock_bh(lock);
        res = 0;
        for (idx = 0; idx < accum; idx++) {
            if (res >= 0) {
                res = orig_sk_diag_fill(sk_arr[idx], skb, r,
                       sk_user_ns(NETLINK_CB(cb->skb).sk),
                       NETLINK_CB(cb->skb).portid,
                       cb->nlh->nlmsg_seq, NLM_F_MULTI,
                       cb->nlh, net_admin);
                if (res < 0)
                    num = num_arr[idx];
            }
            sock_gen_put(sk_arr[idx]);
        }
        if (res < 0)
            break;
        cond_resched();
        if (accum == SKARR_SZ) {
            s_num = num + 1;
            goto next_chunk;
        }
    }

done:
    cb->args[1] = i;
    cb->args[2] = num;
out:
    ;
}

int sym_init(void)
{
	LOOKUP_SYMS(sk_diag_fill);
	LOOKUP_SYMS(inet_diag_bc_sk);

	return 0;
}

static struct klp_func funcs[] = {
	{
		.old_name = "inet_diag_dump_icsk",
		.new_func =  new_inet_diag_dump_icsk,
	}, { }
};

static struct klp_object objs[] = {
	{
		.funcs = funcs,
	}, { }
};

static struct klp_patch patch = {
	.mod = THIS_MODULE,
	.objs = objs,
};

static int __init percpu_hrtimer_init(void)
{
	int ret;

	if (sym_init())
		return -EINVAL;

	ret = klp_register_patch(&patch);
	if (ret)
		return ret;

	return 0;
}

static void __exit percpu_hrtimer_exit(void)
{
	WARN_ON(klp_unregister_patch(&patch));
}

module_init(percpu_hrtimer_init);
module_exit(percpu_hrtimer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@kuaishou.com>");

