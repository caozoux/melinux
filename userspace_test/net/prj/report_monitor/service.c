// server.cpp
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <map>
#include <string>
#include <iostream>

using namespace std;

#define PORT 8080
#define BUFFER_SIZE 512
#define MAX_CLIENTS 100

struct ClientInfo {
    string ip;
    time_t last_active;
    int file_fd;
    string current_file;
    time_t file_start_time;
};

map<int, ClientInfo> clients;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_LOCKED;

string get_hour_filename(time_t timestamp) {
    struct tm *tm_info = localtime(&timestamp);
    char filename[64];
    strftime(filename, sizeof(filename), "%Y%m%d_%H.dat", tm_info);
    return string(filename);
}

string get_client_dir(const string& ip) {
    return string("client_data/") + ip;
}

void create_directory_if_not_exists(const string& path) {
    struct stat st = {0};
    if (stat(path.c_str(), &st) == -1) {
        mkdir(path.c_str(), 0755);
    }
}

int open_hourly_file(const string& client_ip, time_t timestamp) {
    string dir_path = get_client_dir(client_ip);
    create_directory_if_not_exists("client_data");
    create_directory_if_not_exists(dir_path);

    string filename = dir_path + "/" + get_hour_filename(timestamp);
    return open(filename.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
}

void* handle_client(void* arg) {
    int client_fd = *(int*)arg;
    free(arg);

    // Receive client IP
    char ip_buffer[32];
    ssize_t ip_bytes = recv(client_fd, ip_buffer, sizeof(ip_buffer) - 1, 0);
    if (ip_bytes <= 0) {
        close(client_fd);
        return NULL;
    }
    ip_buffer[ip_bytes] = '\0';

    pthread_mutex_lock(&clients_mutex);
    ClientInfo client_info;
    client_info.ip = string(ip_buffer);
    client_info.last_active = time(NULL);
    client_info.file_fd = -1;
    client_info.file_start_time = 0;
    clients[client_fd] = client_info;
    pthread_mutex_unlock(&clients_mutex);

    printf("Client connected: %s\n", ip_buffer);

    char buffer[BUFFER_SIZE];
    while (1) {
        ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            break;
        }

        time_t now = time(NULL);

        pthread_mutex_lock(&clients_mutex);
        ClientInfo& info = clients[client_fd];
        info.last_active = now;

        // Check if we need a new file (new hour or no file yet)
        struct tm* current_tm = localtime(&now);
        time_t current_hour_start = now - (now % 3600);

        if (info.file_fd == -1 || current_hour_start > info.file_start_time) {
            // Close previous file if exists
            if (info.file_fd != -1) {
                close(info.file_fd);
            }

            // Open new file for current hour
            info.file_fd = open_hourly_file(info.ip, now);
            info.file_start_time = current_hour_start;
            info.current_file = get_hour_filename(now);
        }

        // Write data to file
        if (info.file_fd != -1) {
            write(info.file_fd, buffer, bytes_received);
        }

        pthread_mutex_unlock(&clients_mutex);
    }

    // Clean up
    pthread_mutex_lock(&clients_mutex);
    if (clients[client_fd].file_fd != -1) {
        close(clients[client_fd].file_fd);
    }
    clients.erase(client_fd);
    pthread_mutex_unlock(&clients_mutex);

    close(client_fd);
    printf("Client %s disconnected\n", ip_buffer);
    return NULL;
}

void* cleanup_thread(void* arg) {
    while (1) {
        sleep(60); // Check every minute
        time_t now = time(NULL);

        pthread_mutex_lock(&clients_mutex);
        for (auto it = clients.begin(); it != clients.end();) {
            if (now - it->second.last_active > 120) { // 2 minutes timeout
                printf("Cleaning up inactive client: %s\n", it->second.ip.c_str());
                if (it->second.file_fd != -1) {
                    close(it->second.file_fd);
                }
                close(it->first);
                it = clients.erase(it);
            } else {
                ++it;
            }
        }
        pthread_mutex_unlock(&clients_mutex);
    }
    return NULL;
}

int main() {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    // Start cleanup thread
    pthread_t cleanup_tid;
    pthread_create(&cleanup_tid, NULL, cleanup_thread, NULL);

    while (1) {
        int client_fd;
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len)) < 0) {
            perror("accept");
            continue;
        }

        // Handle client in a new thread
        pthread_t tid;
        int* client_fd_ptr = (int*)malloc(sizeof(int));
        *client_fd_ptr = client_fd;
        pthread_create(&tid, NULL, handle_client, client_fd_ptr);
        pthread_detach(tid);
    }

    close(server_fd);
    return 0;
}
