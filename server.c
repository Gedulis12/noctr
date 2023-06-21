#include <stddef.h>
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
    int sockfd; // file descriptor for client-server communication
    struct sockaddr_storage client_addr; // client's ip address
    short int ws_est; // 1 - if the websocket connection is established, 0 - if websocket connection is not established
};

struct ws_frame
{
    unsigned char fin_rsv_opcode;
    unsigned char mask_length;
    unsigned short payload_len;
    unsigned char masking_key[4];
    unsigned char *payload;
};

void *get_in_addr(struct sockaddr *sa);
void *handle_client(void *arg);
int setup_server();
void accept_clients(int sockfd);
void cleanup_client(struct client_data *client);
int is_websocket_request(const char *request);
int send_websocket_handshake_response(int sockfd, const char *request);
void base64_encode(const unsigned char *input, size_t input_length, char *output);
void read_ws_frame(const char *frame);


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
    char buf[BUFFER_SIZE];
    int numbytes;

    inet_ntop(client->client_addr.ss_family, get_in_addr((struct sockaddr *)&(client->client_addr)), s, sizeof s);
    printf("server: got a connection from %s\n", s);

    while ((numbytes = recv(client->sockfd, buf, BUFFER_SIZE -1, 0)) > 0)
    {
        buf[numbytes] = '\0';

        if (client->ws_est != 1)
        {
            if(is_websocket_request(buf) == 1)
            {
                if ((status = send_websocket_handshake_response(client->sockfd, buf)) == 0)
                {
                    client->ws_est = 1;
                }
                printf("send websocket response return status: %d\n", status);
            }
        }
        printf("ws_est value %d\n:", client->ws_est);
        printf("Received: %s\n", buf);
        read_ws_frame(buf);
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

// Check if the message received is HTTP WebSocket handshake request
int is_websocket_request(const char *request)
{
    const char *upgrade_header = "Upgrade: websocket\r\n";
    const char *connection_header = "Connection: ";
    const char *key_header = "Sec-WebSocket-Key";

    const char *upgrade_ptr = strstr(request, upgrade_header);
    if (!upgrade_ptr)
    {
        printf("Upgrade header not found\n");
        return 0;
    }

    const char *connection_ptr = strstr(request, connection_header);
    if (!connection_ptr)
    {
        printf("Connection header not found\n");
        return 0;
    }

    connection_ptr += strlen(connection_header);
    // Check if "Upgrade" is present in the Connection header value
    const char *upgrade_str = "Upgrade";
    const char *upgrade_start = strstr(connection_ptr, upgrade_str);

    if (!upgrade_start)
    {
        printf("Connection header does not contain required value: 'Upgrade'\n");
        return 0;
    }

    if (upgrade_ptr < connection_ptr)
    {
        printf("Upgrade and connection headers are in the wrong order\n");
        return 0;
    }

    const char *key_ptr = strstr(request, key_header);
    if (!key_ptr)
    {
        printf("Key header not found\n");
        return 0;
    }

    return 1;
}

int send_websocket_handshake_response(int sockfd, const char* request)
{
    // Handshake response template
    const char *response_template = "HTTP/1.1 101 Switching Protocols\r\n"
                                    "Upgrade: websocket\r\n"
                                    "Connection: Upgrade\r\n"
                                    "Sec-WebSocket-Accept: %s\r\n"
                                    "\r\n";

    // Header to parse from the request
    const char *key_header = "Sec-WebSocket-Key:";
    const char *key_start = strstr(request, key_header);

    if (key_start == NULL) 
    {
        printf("Invalid WebSocket request\n");
        return 1;
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

    // Concatenate the key from the header with the MAGIC_STRING (WebSocket spec)
    size_t concatenated_key_length = key_length + strlen(MAGIC_STRING) + 1;
    char *concatenated_key = (char *)malloc(concatenated_key_length);

    strcpy(concatenated_key, request_key);
    strcat(concatenated_key, MAGIC_STRING);

    printf("\nConcatenated string: %s\n", concatenated_key);

    // Get SHA1 hash of the concatenated string (WebSocket spwc)
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

    // Base64 encode the SHA1 hash (WebSocket spec)
    char encoded_hash[SHA_DIGEST_LENGTH * 2];
    base64_encode(hash, SHA_DIGEST_LENGTH, encoded_hash);
    printf("Base64 encoded hash: %s\n", encoded_hash);

    // Append the base64 encoded value to the HTTP header
    size_t response_length = strlen(response_template) + strlen(encoded_hash) + 1;
    char* response = (char*)malloc(response_length);
    snprintf(response, response_length, response_template, encoded_hash);
    printf("Response:\n%s\n", response);

    if (send(sockfd, response, response_length, 0) != 0)
    {
        free(request_key);
        free(concatenated_key);
        free(response);
        return 0;
    }

    free(request_key);
    free(concatenated_key);
    free(response);
    return 1;
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


void read_ws_frame(const  char *frame)
{
    struct ws_frame ws_frame;
    unsigned char opcode;
    unsigned char mask_set;
    int mask_start; // byte number at which the mask starts
    int payload_start; // byte number at which the payload starts;
    unsigned char payload_length;
    int len;
    int mask_idx = 0;
    // check if fin bit is 1 and reserved bits are 0
    // bitwise AND the first 4 bits of first byte with 11110000 (240)
    if ((frame[0] & 240) != 128)
    {
        printf("Issue with Fin and reserved bits:\n");
        printf("%d\n", (frame[0] & 240));
        return;
    }

    // check the opcode (message type)
    // bitwise AND the last 4 bits of the first byte with 0000 1111 (15)
    // 0001 (1) for text, 0010 (2) for binary, 1000 (8) to close the connection
    // the server only supports text and connection close types;
    opcode = frame[0] & 15;
    if (opcode == 1)
    {
        ws_frame.fin_rsv_opcode = 129; // 1000 0001
    }
    else if (opcode == 8)
    {
        ws_frame.fin_rsv_opcode = 136; // 1000 1000
    }
    else
    {
        printf("the received opcode is not supported\n");
        return;
    }
    printf("opcode: %d\n", ws_frame.fin_rsv_opcode);

    // check if the mask is applied
    // bitwise AND the first bit of the second byte with 1000 0000 (128)
     mask_set = frame[1] & 128;
     printf("mask bit: %d\n", mask_set);

    // check the payload length
    // bitwise AND the last 7 bits of the second byte with 0111 1111 (127)

    payload_length = frame[1] & 127;
    if (payload_length < 126)
    {
        ws_frame.mask_length = mask_set | payload_length;
        ws_frame.payload_len = 0;

        // set a byte at which the masking-key starts;
        if (mask_set == 128)
        {
            mask_start = 2;
        }
    }
    // if the payload length is >= 126 read the next two bits as the length
    // we combine the bytes of frame[2] and frame[3] by byt-shifting frame[2] to the left by eight places and applying bitwise or with the frame[3]
    else if (payload_length >= 126)
    {
        ws_frame.mask_length = mask_set | payload_length;
        ws_frame.payload_len = ((unsigned short)frame[2] << 8) | frame[3];

        if (mask_set == 128)
        {
            mask_start = 4;
        }
        // messages above the BUFFER_SIZE - 8 are not allowed (8 bytes are reserved for the WebSocket frame headers)
        if (ws_frame.payload_len > (BUFFER_SIZE - 8))
        {
            printf("Message above the %d are not allowed\n", (BUFFER_SIZE - 8));
            return;
        }
    }

    payload_start = mask_start + 4;
    printf("mask_len: %d\n", ws_frame.mask_length);
    printf("actual payload length: %d\n", (ws_frame.mask_length & 127));
    printf("payload len: %d\n", ws_frame.payload_len);

    // assign the masking key bytes to the ws_frame.masking_key
    if (mask_set == 128)
    {
            for (int i = 0; i < 4; i++)
            {
                ws_frame.masking_key[i] = frame[(mask_start + i)];
                printf("masking key #%d: %d\n", i, ws_frame.masking_key[i]);
            }
    }

    // get the actual length of payload and assign it to 'len'
    // use ws_frrame.payload_len is not 0 assign it to length, otherwise parse the length from the
    // second byte by applying bitwise AND with 0111 1111 (127)
    if (ws_frame.payload_len == 0)
    {
        len = (ws_frame.mask_length & 127);
    }
    else
    {
        len = ws_frame.payload_len;
    }

    // read the payload data looping over each byte and unmasking it with corresponding mask
    // unmasking is done by applying bitwise XOR
    for (int i = 0; i < len; i++)
    {
        unsigned char mask = ws_frame.masking_key[mask_idx];
        unsigned char masked_data = frame[(payload_start + i)];
        unsigned char unmasked_data = mask ^ masked_data;
        printf("%c", unmasked_data);

        if (mask_idx != 0 && (mask_idx % 3) == 0)
        {
            mask_idx = -1;
        }

        mask_idx ++;
    }
    printf("\n");
}
