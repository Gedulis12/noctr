#include "../include/server.h"
#include "../include/websocket.h"


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
