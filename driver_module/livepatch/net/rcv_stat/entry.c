// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * livepatch-sample.c - Kernel Live Patching Sample Module
 *
 * Copyright (C) 2014 Seth Jennings <sjenning@redhat.com>
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/livepatch.h>
#include <linux/seq_file.h>
#include <linux/socket.h>
#include <net/sock.h>
#include <linux/skbuff.h>
#include <linux/tcp.h>
#include <linux/ip.h>

#define LOOKUP_SYMS(name) do {                          \
            orig_##name = (void *)kallsyms_lookup_name(#name);      \
            if (!orig_##name) {                     \
                pr_err("kallsyms_lookup_name: %s\n", #name);        \
                return -EINVAL;                     \
            }                               \
        } while (0)

static int (*orig_tcp_rcv_state_process)(struct sock *sk, struct sk_buff *skb);
static int (*old_tcp_rcv_state_process)(struct sock *sk, struct sk_buff *skb);

static int (*orig_tcp_v4_do_rcv)(struct sock *sk, struct sk_buff *skb);
static int (*old_tcp_v4_do_rcv)(struct sock *sk, struct sk_buff *skb);

static int (*orig_tcp_v4_rcv)(struct sk_buff *skb);
static int (*old_tcp_v4_rcv)(struct sk_buff *skb);
 
static int livepatch_tcp_rcv_state_process(struct sock *sk, struct sk_buff *skb)
{                                                                                                                                                                                                                                                                                                                                                    
	int ret;
	const struct tcphdr *th = tcp_hdr(skb);
	const struct inet_sock *inet = inet_sk(sk);
	__be32 dest = inet->inet_daddr;                                                                                                                                                                                                                                                                                                                 
	__be32 src = inet->inet_rcv_saddr;
	__u16 destp = ntohs(inet->inet_dport);
	__u16 srcp = ntohs(inet->inet_sport);

	//ip_hdr(skb)->saddr                                                                                                                                                                                                                                                                                                                               
	//ip_hdr(skb)->daddr

	trace_printk("+ %d %pI4:%04X %pI4:%04X  \n", sk->sk_state,
			&inet->inet_daddr, srcp, &sk->sk_daddr, destp);
    ret = old_tcp_rcv_state_process(sk, skb);
	trace_printk("- %d %pI4:%04X %pI4:%04X  \n", sk->sk_state,
			&inet->inet_daddr, srcp, &sk->sk_daddr, destp);


	//trace_printk("%08X:%04X %08X:%04X  \n",
	//		src, srcp, dest, destp);
	//

	return ret;
}

static int livepatch_tcp_v4_rcv(struct sk_buff *skb)
{                                                                                                                                                                                                                                                                                                                                                    
	int ret;
	const struct iphdr *iph;
	const struct tcphdr *th = tcp_hdr(skb);

	iph = ip_hdr(skb);

	trace_printk("%pI4: %pI4:  \n", 
			&iph->saddr, &iph->daddr);
	ret = old_tcp_v4_rcv(skb);

	return ret;
}

static int livepatch_tcp_v4_do_rcv(struct sock *sk, struct sk_buff *skb)
{                                                                                                                                                                                                                                                                                                                                                    
	int ret;
	const struct tcphdr *th = tcp_hdr(skb);
	const struct inet_sock *inet = inet_sk(sk);
	const struct iphdr *iph;
	__be32 dest = inet->inet_daddr;                                                                                                                                                                                                                                                                                                                 
	__be32 src = inet->inet_rcv_saddr;
	__u16 destp = ntohs(inet->inet_dport);
	__u16 srcp = ntohs(inet->inet_sport);
	iph = ip_hdr(skb);

	//ip_hdr(skb)->saddr                                                                                                                                                                                                                                                                                                                               
	//ip_hdr(skb)->daddr

	trace_printk("%d %pI4:%04X %pI4:%04X  \n", sk->sk_state,
			&iph->saddr, srcp, &iph->daddr, destp);

    ret = old_tcp_v4_do_rcv(sk, skb);

	//trace_printk("%08X:%04X %08X:%04X  \n",
	//		src, srcp, dest, destp);
	//

	return ret;
}

static struct klp_func funcs[] = {
	{
		.old_name = "tcp_rcv_state_process",
		.new_func = livepatch_tcp_rcv_state_process,
	}, 
	{
		.old_name = "tcp_v4_do_rcv",
		.new_func = livepatch_tcp_v4_do_rcv,
	},
	{
		.old_name = "tcp_v4_rcv",
		.new_func = livepatch_tcp_v4_rcv,
	},
	{ }
};

static struct klp_object objs[] = {
	{
		/* name being NULL means vmlinux */
		.funcs = funcs,
	}, { }
};

static struct klp_patch patch = {
	.mod = THIS_MODULE,
	.objs = objs,
};

int sym_init(void)
{
    LOOKUP_SYMS(tcp_rcv_state_process);
    LOOKUP_SYMS(tcp_v4_do_rcv);
    LOOKUP_SYMS(tcp_v4_rcv);
	old_tcp_rcv_state_process = orig_tcp_rcv_state_process + 5;
	old_tcp_v4_do_rcv = orig_tcp_v4_do_rcv + 5;
	old_tcp_v4_rcv= orig_tcp_v4_rcv + 5;

    return 0;
}

static int livepatch_init(void)
{
	if (sym_init())
		return -EINVAL;


	printk("zz %s orig_tcp_rcv_state_process:%lx \n",__func__, (unsigned long)orig_tcp_rcv_state_process);
	printk("zz %s old_tcp_rcv_state_process:%lx \n",__func__, (unsigned long)old_tcp_rcv_state_process);
	return klp_enable_patch(&patch);
}

static void livepatch_exit(void)
{
}

module_init(livepatch_init);
module_exit(livepatch_exit);
MODULE_LICENSE("GPL");
MODULE_INFO(livepatch, "Y");

