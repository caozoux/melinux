#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <fcntl.h> 
#include <arpa/inet.h>
#include <sys/mman.h> 
#include <signal.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <getopt.h>
#include <sstream>
#include <string>
#include <vector>
#include <list>

using namespace std;

enum MODE_NET {
	MODE_NONE,
	MODE_TCP,
	MODE_UDP,
	MODE_XDP,
	MODE_UNIX,
};

enum MODE_NET args_mode_net = MODE_TCP;
// 0: server 1: client
int args_cs_mod=0;
int args_thread_num=1;
char *args_server_ip=NULL;
int args_port=8026;

#define MAP_LENGTH      (1UL<<34) 
#define MAX_TRHEAD  (128)

struct thread_data {
	pthread_t thread;
	int socketfd;
	unsigned long recv_size;
	unsigned long write_size;
    struct sockaddr_in  client_address;
};

//vector<struct thread_data *> thread_server_vect;
list<struct thread_data *> thread_client_vect;
vector<unsigned long> client_timer_report;

void *threadFunc(void *param)
{
	return NULL;
}

static void help()
{
	printf("-m|--mode:     1:tcp/2:udp/3:xdp/4:unix \n");	
	//printf("-f|--file:   specify file name\n");	
	printf("-p|--port:     specify port\n");	
	printf("-s|--size:     package size\n");
	printf("-c|--connect:  connect server\n");
	printf("-S|--server:   server mode\n");
	printf("-t|--time:     run time\n");	
	printf("-T|--threads:  thread number\n");	
}

void handler(int sig)
{
	int i = 0;
	unsigned long total=0;
	for (auto& element : thread_client_vect) {
		unsigned long diff =  element->write_size/1024 - client_timer_report[i];
		printf("stream%d:   %10ld KB\n", i, diff);
		client_timer_report[i++] = element->write_size/1024;
		total += diff;
    }
	printf("total width:    %10ld KB\n", total);
}

int timer_init(int second)
{
    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);

    struct itimerval timer;
    timer.it_value.tv_sec = 5; // 延迟5秒执行
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = second; // 每隔2秒重复执行一次
    timer.it_interval.tv_usec = 0;

    setitimer(ITIMER_REAL, &timer, NULL);
	client_timer_report.resize(args_thread_num);
	return 0;
}

static struct option long_options[] =
{
	{"version", no_argument,	0,	'v'},
	{"help",	no_argument,	0,	'h'},
	{"type",	required_argument,	0,	't'},
	{"connect", required_argument,	0,	'c'},
	{"size",required_argument,	0,	's'},
	{"port",required_argument,	0,	'p'},
	{"time",required_argument,	0,	't'},
	{"threads",required_argument,	0,	'T'},
	{0,0,0,0}
};

#define MAX_PENDING_CONNECTIONS 5
#define BUFFER_SIZE 1024

void *thread_recv(void *param)
{
#define THREAD_BUF_SIZE (0x2000000)
	struct thread_data *data = (struct thread_data *)param;
	int client_socket = data->socketfd;
	void *buffer = malloc(THREAD_BUF_SIZE);

	while (1) {
		// 读取客户端发送的数据
		ssize_t num_bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);

		if (num_bytes == -1) {
			perror("Failed to read from client socket");
			goto out;
		} else if (num_bytes == 0) {
			//printf("Client disconnected\n");
			goto out;
		} 
	}

out:
	close(client_socket);
	delete data;
	return NULL;
}

void *thread_send(void *param)
{
    char message[1024] = {0};
	struct thread_data *data = (struct thread_data *)param;
	int sock = data->socketfd;
	int ret;

	while (1) {
    	ret = send(sock, message, 1024, 0);
		if (ret<=0) 
			goto out;
		data->write_size += ret;
	}

out:
    close(sock);

    return NULL;
}

int mode_tcp_client()
{
	int num = args_thread_num;

	while(num--) {

		int connection_status;
		int sock = socket(AF_INET, SOCK_STREAM, 0);
		struct sockaddr_in server_address;
		server_address.sin_family = AF_INET;
		server_address.sin_port = htons(args_port);   // Change to the port you want to connect to
		server_address.sin_addr.s_addr = inet_addr(args_server_ip);   // Change to the IP address of the server
																	  //
		connection_status = connect(sock, (struct sockaddr *) &server_address, sizeof(server_address));
		if (connection_status != 0) {
			printf("Failed to connect to the server.\n");
			return 1;
		}

		struct thread_data *data = new thread_data();
		data->socketfd=sock;

		pthread_create(&data->thread, NULL, thread_send, data);
		thread_client_vect.push_back(data);
	}
	return 0;
}

int mode_tcp_server()
{
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_len = sizeof(client_address);
    char buffer[BUFFER_SIZE];

    // 创建套接字
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Failed to create server socket");
        exit(EXIT_FAILURE);
    }

    // 配置服务器地址
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(args_port);

    // 绑定套接字和地址
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Failed to bind server socket to address");
        exit(EXIT_FAILURE);
    }

    // 监听套接字
    if (listen(server_socket, MAX_PENDING_CONNECTIONS) == -1) {
        perror("Failed to listen on server socket");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", args_port);

    while (1) {
		struct thread_data *data = new thread_data();
        //接受客户端连接
        if ((client_socket = accept(server_socket, (struct sockaddr *)&data->client_address, &client_address_len)) == -1) {
            perror("Failed to accept client connection");
            exit(EXIT_FAILURE);
        }

		data->socketfd = client_socket;
		pthread_create(&data->thread, NULL, thread_recv, data);
		//thread_server_vect.push_back(data);
    }

    close(server_socket);
    return EXIT_SUCCESS;
}

typedef struct sockaddr    TSockAddr;
typedef struct sockaddr_un TSockAddrUn;
typedef struct sockaddr_in TSockAddrIn;
typedef struct linger      TSoLinger;

#define UNIX_SOCKET_PATH  "/tmp/unix_sock"
//#define UNIX_SOCKET_PATH  "/dev/log"

int mode_unix_server()
{
    int iRet;
	int sockfd;
	char recvbuf[1024];

	struct sockaddr_un serv_unadr;

	bzero(&serv_unadr,sizeof(serv_unadr));

	//serv_unadr.sun_family = AF_UNIX;
	serv_unadr.sun_family = AF_UNIX;
	strcpy(serv_unadr.sun_path, UNIX_SOCKET_PATH);

	signal(SIGPIPE, SIG_IGN);


	/* 创建本地socket */
	sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);//数据包方式
	if ( sockfd <= 0)
	{
	    perror("socket error");
	    return sockfd;
	}

	/* 绑定监听口 */
    //int flag = 1;
    //iRet = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));


    unlink(UNIX_SOCKET_PATH);
	iRet = bind(sockfd, (struct sockaddr *)&serv_unadr, sizeof(struct sockaddr));
	if (iRet != 0)
	{
	    perror("bind error");
		close(sockfd);
		return -1;
	}

	iRet = recv(sockfd, recvbuf, 1024, 0);
	sleep(10);
	while (1) {
		iRet = recv(sockfd, recvbuf, 1024, 0);
		printf("zz %s iRet:%lx \n",__func__, (unsigned long)iRet);
	}
    return sockfd;
}

int mode_unix_client()
{
	int sockfd;
	struct sockaddr_un  unadr;
	socklen_t socklen;
	int len;
	int i;

	bzero(&unadr,sizeof(unadr));

	unadr.sun_family = AF_UNIX;
	strcpy(unadr.sun_path, UNIX_SOCKET_PATH);

	/* 创建本地socket */
	sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);//数据包方式
	if ( sockfd <= 0)
	{
	    perror("CUdpClient:: socket error");
	    return sockfd;
	}

	/* 绑定监听口 */
    //setSocketAttr(sockFd);
	int iRet = connect(sockfd,(struct sockaddr *) &unadr, sizeof(unadr));
	printf("zz %s iRet:%lx \n",__func__, (unsigned long)iRet);
	if (iRet != 0)
	{
	    perror("bind error");
		close(sockfd);
		return -1;
	}

	while(i++) {
		len = send(sockfd, "test", 4, 0);
		printf("zz %d: len:%lx \n",i, (unsigned long)len);
	}

    return sockfd;
}

void update_net_mode(int mode)
{
	switch (mode) {
		case 1:
			args_mode_net = MODE_TCP;
			break;
		case 2:
			args_mode_net = MODE_UDP;
			break;
		case 3:
			args_mode_net = MODE_XDP;
			break;
		case 4:
			args_mode_net = MODE_UNIX;
			break;
		default:
			break;
	}
}

static void entry_server_mode(void)
{
	printf("zz %s %d %d\n", __func__, __LINE__, args_mode_net);
	switch (args_mode_net) {
		case MODE_TCP:
			mode_tcp_server();
			break;
		case MODE_UDP:
			break;
		case MODE_UNIX:
			mode_unix_server();
			break;
		case MODE_XDP:
			break;
		default:
			break;
	}
}

static void entry_client_mode(void)
{
	printf("zz %s %d %d\n", __func__, __LINE__, args_mode_net);
	switch (args_mode_net) {
		case MODE_TCP:
			timer_init(1);
			mode_tcp_client();
			break;
		case MODE_UDP:
			break;
		case MODE_UNIX:
			mode_unix_client();
			break;
		case MODE_XDP:
			break;
		default:
			break;
	}
}

int main(int argc, char *argv[])
{
	int args_size = 0;
	int choice, choice_args;
	int fd;

	args_mode_net = MODE_TCP;

	while (1) {
		int option_index = 0;
		choice = getopt_long( argc, argv, "vhm:t:p:st:T:c:Sp:",
					long_options, &option_index);
		if (choice == -1)
			break;

		switch( choice )
		{
			case 'v':
				break;
	
			case 'h':
				help();
				return 0;
	
			case 'm':
				choice_args = atoi(optarg);
				printf("%d\n", choice_args);
				update_net_mode(choice_args);
				break;

			case 'p':
				args_port = atoi(optarg);
				break;

			case 'c':
				args_cs_mod = 1;
				args_server_ip = (char*)malloc(16);
				memcpy(args_server_ip, optarg, 16);
				break;

			case 't':
				args_thread_num = atoi(optarg);
				break;

			case 'f':
				//memcpy(args_file, optarg, strlen(optarg));
				break;

			case 's':
				break;

			case 'S':
				args_cs_mod = 1;
				break;

			default:
				printf("argsmunt not find\n");
				return -1;
		}
	}

	if (args_cs_mod)
		entry_client_mode();
	else
		entry_server_mode();

	while(1)
		sleep(1);

	return 0;
out:
	return -1;
}

