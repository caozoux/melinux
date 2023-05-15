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
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <getopt.h>
#include <sstream>
#include <string>
#include <vector>
#include <list>

using namespace std;

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
	printf("-t|--type:     tcp/udp/xdp\n");	
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

	printf("zz %s %d \n", __func__, __LINE__);

	while (1) {
    	ret = send(sock, message, 1024, 0);
		if (ret<=0) 
			goto out;
		data->write_size += ret;
	}

out:
    close(sock);
	printf("zz %s %d \n", __func__, __LINE__);

    return NULL;
}

int mode_client()
{
	int num = args_thread_num;

	while(num--) {

		int sock = socket(AF_INET, SOCK_STREAM, 0);
		struct sockaddr_in server_address;
		server_address.sin_family = AF_INET;
		server_address.sin_port = htons(args_port);   // Change to the port you want to connect to
		server_address.sin_addr.s_addr = inet_addr(args_server_ip);   // Change to the IP address of the server
		//server_address.sin_addr.s_addr = inet_addr("192.168.124.2");   // Change to the IP address of the server
																	  //
		//printf("zz %s args_server_ip:%s %d\n",__func__, args_server_ip, args_port);
		int connection_status = connect(sock, (struct sockaddr *) &server_address, sizeof(server_address));
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

int mode_server()
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
        // 接受客户端连接
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

int main(int argc, char *argv[])
{
	int args_size = 0;
	int choice;
	int fd;

	while (1) {
		int option_index = 0;
		choice = getopt_long( argc, argv, "vht:p:s:t:T:c:S",
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
	
			case 'c':
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
				//args_size = atoi(optarg);
				break;

			case 'S':
				mode_server();
				//args_size = atoi(optarg);
				break;

			default:
				return -1;
		}
	}

	// client mode
	if (args_server_ip) {
		timer_init(1);
		mode_client();
	}
#if 0
	if (!strcmp(args_type, "anon")) {
	} else if (!strcmp(args_type, "dirty")) {
	} else if (!strcmp(args_type, "mmap")) {
	} else {
	}
#endif

	//pthread_join(thread_array[i], NULL);

	while(1)
		sleep(1);
	return 0;
out:
	return -1;
}

