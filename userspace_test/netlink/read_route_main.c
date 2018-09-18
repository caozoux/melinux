#include <stdio.h>
#include <string.h>
#include <asm/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
//#include <sys/types.h>
//#include <linux/uio.h>
#include <errno.h>
#ifdef SEQ
struct rtnl_handle
{
	unsigned int seq;
}
#endif
static void parse_rtattr(struct rtattr **tb, int max,
struct rtattr *rta, int len)
{
	while(RTA_OK(rta, len))
	{
		if(rta-> rta_type <= max)
			tb[rta-> rta_type] = rta;
		rta = RTA_NEXT(rta, len);
	}
}

int routeprint( struct sockaddr_nl *snl, struct nlmsghdr *h2)
{
#if 1
	struct rtmsg *rtm;
	struct rtattr *tb[RTA_MAX + 1];
	int len;
	int index;
	int table;
	void* dest;
	void* gate;
	char dest2[100];
	rtm = NLMSG_DATA(h2);//get the data portion of "h2 "
	index = 0;
	dest = NULL;
	gate = NULL;
	table = rtm-> rtm_table;
	len = h2-> nlmsg_len - NLMSG_LENGTH(sizeof(struct rtmsg));
	memset(tb, 0, sizeof tb);
	parse_rtattr(tb, RTA_MAX, RTM_RTA(rtm), len);
	if(tb[RTA_OIF])
		index = *(int *)RTA_DATA(tb[RTA_OIF]);
	if(tb[RTA_DST]){
		dest = RTA_DATA(tb[RTA_DST]);
	// printf( "debug dest\n ");
	}
	else dest = 0;
#if 1
	if(tb[RTA_METRICS]){
		gate = RTA_DATA(tb[RTA_METRICS]);
	}
#else
	if(tb[RTA_GATEWAY]){
	gate = RTA_DATA(tb[RTA_GATEWAY]);
	//iprintf( "debug gate\n ");
	}
#endif
	printf( "family:%d\t ",rtm-> rtm_family);
	printf( "index: %d\t ", index);
	// memcpy(dest2, dest, 4);
	printf( "dest: %d\t ", dest);
	// printf( "dest: %c\t ", dest2[1]);
	// printf( "dest: %c\t ", dest2[2]);
	// printf( "dest: %c\t ", dest2[3]);
	printf( "gate: %d\n ", gate);
#endif
	return 1;
}
#ifdef SEQ
int getroute(int sockfd,struct rtnl_handle *rtnl)
#else
int getroute(int sockfd)
#endif
{
	int i;
	int status, sendsize;
	unsigned char buf[8192];
	struct iovec iov = {(void*)buf, sizeof(buf)};
	struct sockaddr_nl nladdr;
	struct nlmsghdr *h;
	struct
	{
		struct nlmsghdr nlh;
		struct rtgenmsg g;
	}req;


	struct msghdr msg = {
		(void*)&nladdr, sizeof(nladdr),
		&iov, 1, NULL, 0, 0
	};
	nladdr.nl_family = AF_NETLINK;
	req.nlh.nlmsg_len = sizeof(req);
	req.nlh.nlmsg_type = RTM_GETROUTE; //增加或删除内核路由表相应改成RTM_ADDROUTE和RTM_DELROUTE
	req.nlh.nlmsg_flags = NLM_F_ROOT|NLM_F_MATCH|NLM_F_REQUEST;
	req.nlh.nlmsg_pid = 0;
#ifdef SEQ
	req.nlh.nlmsg_seq = ++rtnl-> seq;//may be 0?
#else
	//int i;
	//if (i > 4096) i = 1;
	req.nlh.nlmsg_seq = 1;
#endif
	req.g.rtgen_family = AF_INET;
	printf( "sockfd: %d\n ", sockfd);
	if((sendsize=sendto(sockfd, (void*)&req, sizeof(req), 0,
		(struct sockaddr*)&nladdr, sizeof(nladdr))) < 0) {
		perror( "sendto");
		return -1;
	}
	printf( "sendsize= %d\n ",sendsize);
	if((status=recvmsg(sockfd, &msg, 0)) < 0){
		perror( "recvmsg");
		return -1;
	}
	printf( "status= %d\n ",status);
#if 1 //segmentation fault
	for(h = (struct nlmsghdr*)buf; NLMSG_OK(h, status);
			h = NLMSG_NEXT(h, status))
	{
		if(h-> nlmsg_type == NLMSG_DONE)
		{
			printf( "finish reading\n ");
			return 1;
		}
		if(h-> nlmsg_type == NLMSG_ERROR)
		{
			printf( "h:nlmsg ERROR ");
			return -1;
		}
		routeprint(&nladdr, h);
	}
#endif
// printf( "Can 't convert 'h '\n ");
// routeprint(h);
return 1;
}

int main()
{
	int sockfd;
	int cnt=0;
#ifdef SEQ
	struct rtnl_handle rth;
#endif
	struct sockaddr_nl nladdr;
	if ((sockfd = socket(AF_NETLINK, SOCK_RAW,
		NETLINK_ROUTE)) <0){
		perror( "netlink socket ");
		return -1;
	}
	nladdr.nl_family = AF_NETLINK;
	nladdr.nl_pad = 0;
	nladdr.nl_pid = 0;
	nladdr.nl_groups = RTMGRP_LINK|RTMGRP_IPV4_ROUTE|
	RTMGRP_IPV4_IFADDR;
	if (bind(sockfd, (struct sockaddr*)&nladdr,
		sizeof(nladdr)) < 0){
		perror( "bind ");
		close(sockfd);
		return -1;
	}
	while(1) {

		printf("zz %s cnt:%08x \n",__func__, (int)cnt++);
#ifdef SEQ
		if (getroute(sockfd, &rth) < 0) {
#else
		if (getroute(sockfd) < 0) {
#endif
			perror( "can 't get route\n ");
			sleep(5);
			continue;
		}
	}

	return 1;
}
