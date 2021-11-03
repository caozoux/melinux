#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/epoll.h>
#include<curl/curl.h>
 
typedef struct _global_info {
    int epfd;
    CURLM *multi;
} global_info;
 
typedef struct _easy_curl_data {
    CURL *curl;
    char data[1024];
} easy_curl_data;
 
typedef struct _multi_curl_sockinfo {
    curl_socket_t fd;
    CURL *cp;
} multi_curl_sockinfo;
 
static global_info g;
char curl_cb_data[1024] = {0};
static multi_curl_sockinfo  *fpd =NULL;
 
static size_t write_cb(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize;
    realsize = size * nmemb;

	printf("zz %s realsize:%lx \n",__func__, (unsigned long)realsize);
    return realsize;
}

static int sock_cb (CURL *e, curl_socket_t s, int what, void *cbp, void *sockp)
{
    struct epoll_event ev = {0};
 
    global_info * g = (global_info *) cbp;
    multi_curl_sockinfo  *fdp = (multi_curl_sockinfo *) sockp;
 
	printf("zz %s g:%lx \n",__func__, (unsigned long)g);
    if (what == CURL_POLL_REMOVE) {
        if (fdp) {
            free(fdp);
        }
        epoll_ctl(g->epfd, EPOLL_CTL_DEL, s, &ev);
    } else {
        if (what == CURL_POLL_IN) {
            ev.events |= EPOLLIN;
        } else if (what == CURL_POLL_OUT) {
            ev.events |= EPOLLOUT;
        } else if (what == CURL_POLL_INOUT) {
            ev.events |= EPOLLIN | EPOLLOUT;
        }
 
        if (!fpd) {
            fpd = (multi_curl_sockinfo *)malloc(sizeof(multi_curl_sockinfo));
            fpd->fd = s;
            fpd->cp = e;
 
			printf("zz %s g:%lx \n",__func__, (unsigned long)g);
            epoll_ctl(g->epfd, EPOLL_CTL_ADD, s, &ev);
            curl_multi_assign(g->multi, s, &ev);
        }
 
    }
    return 0;
}
 
static void set_curl_opt(CURL *curl)
{
    //set curl options..
    //other options..
}
 
int main(int argc, char *argv[])
{
    int running_count;
    struct epoll_event events[10];
    char *urls = "http://www.cppblog.com/deane/articles/165218.html";
    int nfds;
    int i=0;
 
    curl_global_init(CURL_GLOBAL_ALL);
    memset(&g, 0, sizeof(global_info));
    g.epfd = epoll_create(10);
    g.multi = curl_multi_init();
 
    for(;i<1;i++) {
        CURL *curl;
        curl = curl_easy_init();
        curl_easy_setopt(curl, CURLOPT_URL, urls);
		printf("zz %s g:%lx \n",__func__, (unsigned long)&g);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, curl_cb_data);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);

		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		//curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 3);

        curl_multi_add_handle(g.multi, curl);
    }
 
    curl_multi_setopt(g.multi, CURLMOPT_SOCKETFUNCTION, sock_cb);
    curl_multi_setopt(g.multi, CURLMOPT_SOCKETDATA, &g);
 
    while (CURLM_CALL_MULTI_PERFORM == curl_multi_socket_action(g.multi, CURL_SOCKET_TIMEOUT, 0, &running_count));
 
	printf("zz %s running_count:%lx \n",__func__, (unsigned long)running_count);
    if (running_count) {
        do {
            nfds = epoll_wait(g.epfd, events, 10, 500);
            if(nfds > 0) {
                int z=0;
                for (;z<nfds; z++) {
					//printf("zz %s events:%lx \n",__func__, (unsigned long)events[i].events);
                    if (events[i].events & EPOLLIN) {
						printf("zz %s %d \n", __func__, __LINE__);
                        curl_multi_socket_action(g.multi, CURL_CSELECT_IN, events[i].data.fd, &running_count);
                    } else if (events[i].events & EPOLLOUT) {
						printf("zz %s %d \n", __func__, __LINE__);
                        curl_multi_socket_action(g.multi, CURL_CSELECT_OUT, events[i].data.fd, &running_count);
                    } 
                }
				//break;
            }
        } while (running_count);
    }
 
    curl_global_cleanup();
    return 0;
}

