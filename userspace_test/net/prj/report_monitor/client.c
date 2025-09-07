// client.cpp
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>

#define PORT 8080
#define BUFFER_SIZE 512
#define SERVER_IP "127.0.0.1"

int connect_to_server(const char* server_ip, int port) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    
    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }
    
    return sock;
}

void generate_random_data(char* buffer, int size) {
    static int seed_initialized = 0;
    if (!seed_initialized) {
        srand(time(NULL));
        seed_initialized = 1;
    }
    
    for (int i = 0; i < size; i++) {
        buffer[i] = (char)(rand() % 256);
    }
}

int main(int argc, char *argv[]) {
    const char* server_ip = SERVER_IP;
    if (argc > 1) {
        server_ip = argv[1];
    }
    
    char local_ip[32];
    // Get local IP - for simplicity, we'll use a placeholder
    strcpy(local_ip, "127.0.0.1");
    
    int sock = -1;
    int connected = 0;
    
    while (1) {
        if (!connected) {
            sock = connect_to_server(server_ip, PORT);
            if (sock < 0) {
                printf("Failed to connect to server. Retrying in 10 seconds...\n");
                sleep(10);
                continue;
            }
            
            // Send local IP to server
            if (send(sock, local_ip, strlen(local_ip), 0) <= 0) {
                printf("Failed to send IP to server\n");
                close(sock);
                sleep(10);
                continue;
            }
            
            connected = 1;
            printf("Connected to server. Sending data every minute...\n");
        }
        
        // Generate and send 512 bytes of data
        char buffer[BUFFER_SIZE];
        generate_random_data(buffer, BUFFER_SIZE);
        
        if (send(sock, buffer, BUFFER_SIZE, 0) <= 0) {
            printf("Failed to send data. Reconnecting...\n");
            close(sock);
            connected = 0;
            sleep(5);
            continue;
        }
        
        printf("Sent 512 bytes of data at %ld\n", time(NULL));
        
        // Wait for 1 minute
        sleep(60);
    }
    
    if (connected) {
        close(sock);
    }
    
    return 0;
}
