#ifndef WEBSOCKET
#define WEBSOCKET

#include "server.h"
#include <openssl/sha.h>
#include <openssl/evp.h>

#define MAGIC_STRING "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

struct ws_frame
{
    short mask_start;
    short payload_start;
    short mask_idx;
    unsigned short payload_length;
    unsigned char *payload;
    unsigned char opcode;
    unsigned char mask_bit;
    unsigned char masking_key[4];
    unsigned char fin_rsv;
};

int is_websocket_request(const char *request);
int send_websocket_handshake_response(int sockfd, const char *request);
int read_ws_frame(struct client_data *client, const unsigned char *frame, char *unmasked_payload);
int send_websocket_frame(struct client_data *client, const char *payload, size_t payload_len);
void base64_encode(const unsigned char *input, size_t input_length, char *output);

#endif
