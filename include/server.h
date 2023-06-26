#ifndef SERVER
#define SERVER

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define PORT "6969"
#define BACKLOG 10
#define BUFFER_SIZE 8192

struct client_data
{
    int sockfd; // file descriptor for client-server communication
    struct sockaddr_storage client_addr; // client's ip address
    short int ws_est; // 1 - if the websocket connection is established, 0 - if websocket connection is not established
};

void *get_in_addr(struct sockaddr *sa);
void *handle_client(void *arg);
int setup_server();
void accept_clients(int sockfd);
void cleanup_client(struct client_data *client);

#endif
