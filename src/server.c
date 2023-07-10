#include "../include/server.h"
#include "../include/websocket.h"
#include "../include/event.h"

void *get_in_addr(struct sockaddr *sa)
{
    // if IPV4
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    // if IPV6
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


void *handle_client(void *arg)
{
    struct client_data *client = (struct client_data *)arg;
    client->ws_est = 0;
    int status;
    char s[INET6_ADDRSTRLEN];
    unsigned char buf[BUFFER_SIZE];
    char ws_recv[BUFFER_SIZE];
    int numbytes;

    inet_ntop(client->client_addr.ss_family, get_in_addr((struct sockaddr *)&(client->client_addr)), s, sizeof s);
    printf("server: got a connection from %s\n", s);

    while ((numbytes = recv(client->sockfd, buf, BUFFER_SIZE -1, 0)) > 0)
    {
        buf[numbytes] = '\0';

        if (client->ws_est != 1)
        {
            if(is_websocket_request((const char*)buf) == 1)
            {
                if ((status = send_websocket_handshake_response(client->sockfd, (const char*)buf)) == 0)
                {
                    client->ws_est = 1;
                }
                printf("send websocket response return status: %d\n", status);
            }
        }

        if (client->ws_est == 1)
        {
            if ((status = read_ws_frame(client, buf, ws_recv)) == 2)
            {
                close(client->sockfd);
                free(client);
                pthread_exit(NULL);
            }
            if (status == 0)
            {
                send_websocket_frame(client, ws_recv, strlen(ws_recv));
            }
        }
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
