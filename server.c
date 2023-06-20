#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <openssl/sha.h>
#include <openssl/evp.h>

#define PORT "6969"
#define BACKLOG 10
#define BUFFER_SIZE 8192
#define MAGIC_STRING "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

struct client_data
{
    int sockfd;
    struct sockaddr_storage client_addr;
};

void *get_in_addr(struct sockaddr *sa);
void *handle_client(void *arg);
int setup_server();
void accept_clients(int sockfd);
void cleanup_client(struct client_data *client);
int is_websocket_request(const char *request);
void send_websocket_handshake_response(int sockfd, const char *request);
void base64_encode(const unsigned char *input, size_t input_length, char *output);


int main(void)
{
    int sockfd = setup_server();
    if (sockfd == -1)
    {
        fprintf(stderr, "Failed to set up the server.\n");
        return 1;
    }

    accept_clients(sockfd);

    return 0;
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void *handle_client(void *arg)
{
    struct client_data *client = (struct client_data *)arg;
    char s[INET6_ADDRSTRLEN];
    char buf[BUFFER_SIZE];
    int numbytes;

    inet_ntop(client->client_addr.ss_family, get_in_addr((struct sockaddr *)&(client->client_addr)), s, sizeof s);
    printf("server: got a connection from %s\n", s);

    while ((numbytes = recv(client->sockfd, buf, BUFFER_SIZE -1, 0)) > 0)
    {
        buf[numbytes] = '\0';
        if (is_websocket_request(buf) == 1)
        {
            send_websocket_handshake_response(client->sockfd, buf);
        }
        printf("Received: %s\n", buf);
    }

    if (numbytes == 0)
    {
        printf("client on socket %d disconnected\n", client->sockfd);
    }
    else
    {
        perror("recv");
    }

    close(client->sockfd);
    free(client);
    pthread_exit(NULL);

}

int setup_server()
{
    int sockfd;
    int rv;
    struct addrinfo hints, *servinfo, *p;
    int yes = 1;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            perror("server: bind");
            continue;
        }

        break;
    }


    freeaddrinfo(servinfo);

    if (p == NULL)
    {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    return sockfd;
}

void accept_clients(int sockfd)
{
    pthread_t thread;
    struct sockaddr_storage client_addr;
    socklen_t sin_size;
    int new_fd;

    while (1)
    {
        sin_size = sizeof client_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
        if (new_fd == -1)
        {
            perror("accept");
            continue;
        }

        struct client_data *client = (struct client_data *)malloc(sizeof(struct client_data));
        client->sockfd = new_fd;
        client->client_addr = client_addr;

        if (pthread_create(&thread, NULL, handle_client, (void *)client) != 0)
        {
            perror("pthread create");
            close(new_fd);
            free(client);
        }
    }
}

void cleanup_client(struct client_data *client)
{
    close(client->sockfd);
    free(client);
}

int is_websocket_request(const char *request)
{
    const char *upgrade_header = "Upgrade: websocket\r\n";
    const char *connection_header = "Connection: Upgrade\r\n";
    const char *key_header = "Sec-WebSocket-Key";

    const char *upgrade_ptr = strstr(request, upgrade_header);
    if (!upgrade_ptr)
    {
        return 0;
    }

    const char *connection_ptr = strstr(request, connection_header);
    if (!connection_ptr)
    {
        return 0;
    }

    if (upgrade_ptr < connection_ptr)
    {
        return 0;
    }

    const char *key_ptr = strstr(request, key_header);
    if (!key_ptr)
        return 0;

    return 1;
}

void send_websocket_handshake_response(int sockfd, const char* request)
{
    const char *response_template = "HTTP/1.1 101 Switching Protocols\r\n"
                                    "Upgrade: websocket\r\n"
                                    "Connection: Upgrade\r\n"
                                    "Sec-WebSocket-Accept: %s\r\n"
                                    "\r\n";

    const char *key_header = "Sec-WebSocket-Key:";
    const char *key_start = strstr(request, key_header);

    if (key_start == NULL) 
    {
    printf("Invalid WebSocket request\n");
    return;
    }

    key_start += strlen(key_header);
    while (*key_start == ' ')
    {
        key_start ++;
    }

    const char *key_end = strchr(key_start, '\r');

    size_t key_length = key_end - key_start;
    char *request_key = (char *)malloc(key_length + 1);
    strncpy(request_key, key_start, key_length);
    request_key[key_length] = '\0';

    size_t concatenated_key_length = key_length + strlen(MAGIC_STRING) + 1;
    char *concatenated_key = (char *)malloc(concatenated_key_length);

    strcpy(concatenated_key, request_key);
    strcat(concatenated_key, MAGIC_STRING);

    printf("\nConcatenated string: %s\n", concatenated_key);

    concatenated_key_length = strlen(concatenated_key);
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char *)concatenated_key, concatenated_key_length, hash);

    int i = 0;
    printf("Hash: ");
    while(i < SHA_DIGEST_LENGTH)
    {
        printf("%02x", hash[i]);
        i ++;
    }
    printf("\n");
    char encoded_hash[SHA_DIGEST_LENGTH * 2];
    base64_encode(hash, SHA_DIGEST_LENGTH, encoded_hash);
    printf("Base64 encoded hash: %s\n", encoded_hash);
    free(request_key);
    free(concatenated_key);

}

void base64_encode(const unsigned char *input, size_t input_length, char *output)
{
    EVP_ENCODE_CTX *ctx = EVP_ENCODE_CTX_new();
    int output_length;
    EVP_EncodeInit(ctx);
    EVP_EncodeUpdate(ctx, (unsigned char *)output, &output_length, input, input_length);
    EVP_EncodeFinal(ctx, (unsigned char *)&output[output_length], &output_length);
    EVP_ENCODE_CTX_free(ctx);
}
