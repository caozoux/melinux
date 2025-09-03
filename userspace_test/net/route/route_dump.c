// compile with: gcc dump_cloned_routes.c -o dump_cloned_routes

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>

// 辅助函数：将 ifindex 转换为 ifname
const char *ifindextoname(unsigned int ifindex) {
    static char buf[IF_NAMESIZE];
    if (if_indextoname(ifindex, buf) != NULL)
        return buf;
    return "unknown";
}

// 辅助函数：将 IP 地址（in_addr）转为字符串
char *inet_ntoa_custom(struct in_addr in) {
    static char buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &in, buf, INET_ADDRSTRLEN);
    return buf;
}

// 辅助函数：打印路由信息
void print_route(struct nlmsghdr *nlh) {
    struct rtmsg *rtm = (struct rtmsg *)NLMSG_DATA(nlh);
    struct rtattr *rta = RTM_RTA(rtm);
    int rtl = RTM_PAYLOAD(nlh);

    char dst[INET_ADDRSTRLEN] = "0.0.0.0";
    char gw[INET_ADDRSTRLEN] = "0.0.0.0";
    char *ifname = "unknown";
    unsigned int dst_len = 0;
    unsigned int gateway = 0;
    unsigned int ifindex = 0;

    // 遍历所有属性
    for (; RTA_OK(rta, rtl); rta = RTA_NEXT(rta, rtl)) {
        switch (rta->rta_type) {
            case RTA_DST:
                struct in_addr *d = (struct in_addr *)RTA_DATA(rta);
                inet_ntop(AF_INET, d, dst, sizeof(dst));
                dst_len = rtm->rtm_dst_len;
                break;
            case RTA_GATEWAY:
                struct in_addr *g = (struct in_addr *)RTA_DATA(rta);
                inet_ntop(AF_INET, g, gw, sizeof(gw));
                gateway = g->s_addr;
                break;
            case RTA_OIF:
                ifindex = *(unsigned int *)RTA_DATA(rta);
                ifname = ifindextoname(ifindex);
                break;
        }
    }

    // 只处理 IPv4 路由 (rtm_family == AF_INET)
    if (rtm->rtm_family != AF_INET) {
        return;
    }

    // 判断是否是 cloned 路由
    int is_cloned = !!(rtm->rtm_flags & RTM_F_CLONED);

    printf("Route: dst=%s/%u, gw=%s, if=%s(%u), flags=0x%x",
           dst, dst_len,
           (strcmp(gw, "0.0.0.0") == 0) ? "*" : gw,
           ifname, ifindex,
           rtm->rtm_flags);

    if (is_cloned) {
        printf("  <--- CLONED ROUTE");
    }

    printf("\n");
}

int main() {
    struct sockaddr_nl sa;
    int fd, len;
    char buf[4096];
    struct nlmsghdr *nlh;
    struct rtmsg *rtm;

    // 创建 Netlink 套接字
    fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (fd < 0) {
        perror("socket");
        return 1;
    }

    memset(&sa, 0, sizeof(sa));
    sa.nl_family = AF_NETLINK;
    sa.nl_groups = 0;

    if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        perror("bind");
        close(fd);
        return 1;
    }

    // 构造 RTM_GETROUTE 请求消息
    nlh = (struct nlmsghdr *)buf;
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    nlh->nlmsg_type = RTM_GETROUTE;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    nlh->nlmsg_seq = 1;
    nlh->nlmsg_pid = getpid();

    rtm = (struct rtmsg *)NLMSG_DATA(nlh);
    rtm->rtm_family = AF_INET;  // 只查询 IPv4 路由，如需要 IPv6 则改为 AF_INET6
    rtm->rtm_dst_len = 0;
    rtm->rtm_src_len = 0;
    rtm->rtm_tos = 0;
    rtm->rtm_table = RT_TABLE_MAIN;  // 主路由表，也可以遍历其他表
    rtm->rtm_protocol = 0;
    rtm->rtm_scope = 0;
    rtm->rtm_type = 0;
    rtm->rtm_flags = 0;

    // 发送请求
    if (send(fd, nlh, nlh->nlmsg_len, 0) < 0) {
        perror("send");
        close(fd);
        return 1;
    }

    // 接收并处理回复
    while ((len = recv(fd, buf, sizeof(buf), 0)) > 0) {
        for (nlh = (struct nlmsghdr *)buf; NLMSG_OK(nlh, len); nlh = NLMSG_NEXT(nlh, len)) {
            if (nlh->nlmsg_type == NLMSG_DONE) {
                goto done;
            }
            if (nlh->nlmsg_type == NLMSG_ERROR) {
                fprintf(stderr, "Netlink error message received!\n");
                goto done;
            }
            if (nlh->nlmsg_type == RTM_NEWROUTE) {
                print_route(nlh);
            }
        }
    }

done:
    close(fd);
    return 0;
}
