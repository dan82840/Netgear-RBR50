/* 
 * Copyright (C) 2003, CyberTAN Corporation
 * All Rights Reserved.
 *
 * Description:
 *   This is kernel module for copy and transmit http packets to user space .
 *
 *   The module follows the Netfilter framework, called extended packet
 *   matching modules.
 *
 */

#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/version.h>
#include <linux/netlink.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31)
#include <linux/netfilter/x_tables.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
#define IPT_CONTINUE	XT_CONTINUE
#endif
#define ipt_register_target(target)		xt_register_target(target)
#define ipt_unregister_target(target)		xt_unregister_target(target)

#else
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/ip_conntrack.h>
#include <linux/netfilter_ipv4/ip_conntrack_core.h>
#include <linux/netfilter_ipv4/ip_conntrack_tuple.h>
#endif

#if 0
#define DEBUGP printk
#else
#define DEBUGP(format, args...)
#endif
#define NIPQUAD(addr) \
	((unsigned char *)&addr)[0], \
	((unsigned char *)&addr)[1], \
	((unsigned char *)&addr)[2], \
	((unsigned char *)&addr)[3]
#define NIPQUAD_FMT "%u.%u.%u.%u"
#define N 255
int dye_ip[N] = {0};

#define NETLINK_XHTTP 22
struct sock *nl_sk = NULL;

typedef struct http_nl_packet_msg {
	size_t data_len;
	size_t http_len;
	unsigned char data[0];
} http_nl_packet_msg_t;

//Hex Char "User-Agent"
char UserAgentHex[] = {0x55, 0x73, 0x65, 0x72, 0x2d, 0x41, 0x67, 0x65, 0x6e, 0x74};
int ua_len = sizeof(UserAgentHex)/sizeof(UserAgentHex[0]);
//Hex Char "Accept"
char AcceptHex[] = {0x41, 0x63, 0x63, 0x65, 0x70, 0x74};
int ac_len = sizeof(AcceptHex)/sizeof(AcceptHex[0]);

static unsigned int ipt_xhttp_target(
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31)
		struct sk_buff *skb,
		const struct xt_target_param *par
#else
		struct sk_buff **pskb,
		const struct net_device *in,
		const struct net_device *out,
		unsigned int hooknum,
		const void *targinfo,
		void *userinfo
#endif
		)
{
	int i, j, num = 0, ua_flag = 0, start;
	char _saddr[32];
	struct iphdr *iph;
	struct tcphdr *tcph;
	struct tcphdr _tcph;
	int protoff;
	struct sk_buff *skb_nl;
	
	http_nl_packet_msg_t *pm;
	struct nlmsghdr *nlh;
	size_t size, copy_len;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31)
	iph = ip_hdr(skb);
#else
	struct sk_buff *skb = *pskb;
	iph = skb->nh.iph;
#endif
	if (iph->protocol != IPPROTO_TCP) {
		printk("XHTTP: Not TCP\n");
		return IPT_CONTINUE;
	}

	protoff = iph->ihl * 4;
	tcph = skb_header_pointer(skb, protoff, sizeof(_tcph), &_tcph);
	if (tcph == NULL) {
		printk("XHTTP: tcph NULL\n");
		return IPT_CONTINUE;
	}

//	if (ntohs(tcph->dest) != 0x50) {
//		DEBUGP("XHTTP: Not HTTP\n");
//		return IPT_CONTINUE;
//	}

//	if (((unsigned char *)tcph)[13] != 0x18) {
//		DEBUGP("XHTTP: Not PSH|ACK\n");
//		return IPT_CONTINUE;
//	}
	sprintf(_saddr, NIPQUAD_FMT, NIPQUAD(iph->saddr));
	char * const delim = ".";
	char *token, *cur = _saddr;

	for ( i = 0; i < 4; i++)
		token = strsep(&cur, delim);

	kstrtoint(token, 10, &num);
	if ( dye_ip[num] < 3 ){
		for( i = 0; i < skb->len; i++)
		{
			if(!ua_flag)
				for( j = 0; ((*(skb->data + i + j)) == UserAgentHex[j]); j++ )
				{
					if( j > ua_len - 2 ){
						start = i;
						ua_flag = 1;
						break;
					}
				}
			else{
				for( j = 0; (*(skb->data + i + j)) == AcceptHex[j]; j++ )
				{
					if( j > ac_len - 2 )
						if((i + j - start) > 128){
							goto send;
						}
						else{
							return IPT_CONTINUE;
						}
				}
			}
		}
		DEBUGP("XHTTP: Not contain User-Agent!!!!!!\n");
		return IPT_CONTINUE;
	}
	else{
		DEBUGP("XHTTP: Have send 3 HTTP packets contains User-Agent in 15 minutes!!!!!!\n");
		return IPT_CONTINUE;
	}

send:
	dye_ip[num]++ ;
	
	DEBUGP("XHTTP: Catch HTTP, src addr: %u.%u.%u.%u, dest addr:%u.%u.%u.%u, len:%d, dest port:%u, flags:%x\n", NIPQUAD(iph->saddr), NIPQUAD(iph->daddr), ntohs(iph->tot_len), ntohs(tcph->dest), ((unsigned char *)tcph)[13]);
	
	copy_len = skb->len;
	size = NLMSG_SPACE(sizeof(*pm) + copy_len);
	
	skb_nl = alloc_skb(4096, GFP_ATOMIC); 
	if(!skb_nl)
	{
		printk("XHTTP: Can't alloc whole buffer of size %ub!\n", 4096);
		skb_nl = alloc_skb(size, GFP_ATOMIC);
		if(!skb_nl)
		{
			printk("XHTTP: Can't alloc buffer of size %ub!\n", 4096);
			return IPT_CONTINUE;	
		}
	}

	nlh = nlmsg_put(skb_nl, 0, 0, 0, (size - NLMSG_ALIGN(sizeof(*nlh))),0);
	nlh->nlmsg_len = sizeof(*pm) + skb->len;
	pm = NLMSG_DATA(nlh);
	
	pm->data_len = skb->len;
	pm->http_len = skb->len - protoff - 20;
	skb_copy_bits(skb, 0, pm->data, skb->len);
/*
	if(printk_ratelimit())
	{
		for(i=0; i<100; i++) 
		{
			printk("%02x ", skb->data[i]);
			if (i % 16 == 15)
				printk("\n");
		}
		printk("skb->protocol = 0x%04x\n", ntohs(skb->protocol));
	}
*/	
	NETLINK_CB(skb_nl).dst_group = 1;
	netlink_broadcast(nl_sk, skb_nl, 0, 1, GFP_ATOMIC);
	
	return IPT_CONTINUE;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31)
static bool ipt_xhttp_check(const struct xt_tgchk_param *par)
#else
static int ipt_xhttp_check(const char *tablename,
		const struct ipt_entry *e,
		void *targinfo,
		unsigned int targinfosize,
		unsigned int hook_mask)
#endif
{
	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
static struct xt_target ipt_xhttp_reg = {
#else
static struct ipt_target ipt_xhttp_reg = {
#endif
	.name		= "XHTTP",
	.target		= ipt_xhttp_target,
	.checkentry	= ipt_xhttp_check,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31)
	.family		= NFPROTO_IPV4,
	.targetsize	= sizeof(int),
#endif
	.me			= THIS_MODULE,
};

static int netlink_init(void) {
	nl_sk = netlink_kernel_create(&init_net, 7, NULL);
	if (!nl_sk) {
		printk("XHTTP netlik: cannot create netlink socket.\n");
		return -EIO;
	}
	printk("XHTTP netlink: create netlink OK.\n");
	return 0;
}

static int netlink_fini(void) {
	if (nl_sk != NULL) {
		netlink_kernel_release(nl_sk);
	}
	printk("XHTTP netlink: remove OK.\n");
}

static int __init init(void)
{
	netlink_init();

	return ipt_register_target(&ipt_xhttp_reg);
}

static void __exit fini(void)
{
	netlink_fini();

	ipt_unregister_target(&ipt_xhttp_reg);
}

module_init(init);
module_exit(fini);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("");
MODULE_DESCRIPTION("Firewall and NAT");
