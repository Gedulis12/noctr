#include "websocket.h"


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
                                    "Sec-WebSocket-Accept: %s\r\n";

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


    // Get SHA1 hash of the concatenated string (WebSocket spwc)
    concatenated_key_length = strlen(concatenated_key);
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char *)concatenated_key, concatenated_key_length, hash);

    // Base64 encode the SHA1 hash (WebSocket spec)
    char encoded_hash[SHA_DIGEST_LENGTH * 2];
    base64_encode(hash, SHA_DIGEST_LENGTH, encoded_hash);

    // Append the base64 encoded value to the HTTP header
    char *response = NULL;
    int response_length = snprintf(NULL, 0, response_template, encoded_hash);
    if (response_template < 0)
    {
        free(request_key);
        free(concatenated_key);
        return 1;
    }
    response = (char*)malloc(response_length + 1);
    snprintf(response, response_length + 1, response_template, encoded_hash);

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


int read_ws_frame(struct client_data *client, const unsigned char *frame, char *unmasked_payload)
{
    struct ws_frame ws_frame;

    // check if fin bit is 1 and reserved bits are 0
    // bitwise AND the first 4 bits of first byte with 11110000 (240)
    if ((frame[0] & 240) != 128)
    {
        printf("Issue with Fin and reserved bits:\n");
        printf("%d\n", (frame[0] & 240));
        return 1;
    }

    // check the opcode (message type)
    // bitwise AND the last 4 bits of the first byte with 0000 1111 (15)
    // 0001 (1) for text, 0010 (2) for binary, 1000 (8) to close the connection
    // the server only supports text and connection close types;
    ws_frame.opcode = frame[0] & 15;
    if (ws_frame.opcode == 8)
    {
        printf("client issued a disconnect\n");
        client->ws_est = 0;
        return 2;
    }
    else if (ws_frame.opcode != 1)
    {
        printf("received opcode: %x is not supported.\n", ws_frame.opcode);
        return 1;
    }

    // check if the mask is applied
    // bitwise AND the first bit of the second byte with 1000 0000 (128)
     ws_frame.mask_bit = frame[1] & 128;

    // check the payload length
    // bitwise AND the last 7 bits of the second byte with 0111 1111 (127)
    if ((frame[1] & 127) < 126)
    {
        ws_frame.payload_length = (frame[1] & 127);

        // set a byte at which the masking-key starts;
        if (ws_frame.mask_bit == 128)
        {
            ws_frame.mask_start = 2;
        }
    }
    // if the payload length is >= 126 read the next two bits as the length
    // we combine the bytes of frame[2] and frame[3] by byt-shifting frame[2] to the left by eight places and applying bitwise or with the frame[3]
    else if ((frame[1] & 127) == 126)
    {
        ws_frame.payload_length = ((unsigned short)frame[2] << 8) | frame[3];

        if (ws_frame.mask_bit == 128)
        {
            ws_frame.mask_start = 4;
        }
        // messages above the (BUFFER_SIZE - 8) are not allowed (8 bytes are reserved for the WebSocket frame headers)
        if (ws_frame.payload_length > (BUFFER_SIZE - 8))
        {
            printf("Message above the %d are not allowed, received size: %d\n", (BUFFER_SIZE - 8), ws_frame.payload_length);
            return 1;
        }
    }

    ws_frame.payload_start = ws_frame.mask_start + 4;

    // assign the masking key bytes to the ws_frame.masking_key
    if (ws_frame.mask_bit == 128)
    {
            for (int i = 0; i < 4; i++)
            {
                ws_frame.masking_key[i] = frame[(ws_frame.mask_start + i)];
            }
    }

    // read the payload data looping over each byte and unmasking it with corresponding mask
    // unmasking is done by applying bitwise XOR
    (unmasked_payload)[ws_frame.payload_length] = '\0';
    ws_frame.mask_idx = 0;

    for (int i = 0; i < ws_frame.payload_length; i++)
    {
        unsigned char mask = ws_frame.masking_key[ws_frame.mask_idx];
        unsigned char masked_data = frame[(ws_frame.payload_start + i)];
        unsigned char unmasked_data = mask ^ masked_data;
        (unmasked_payload)[i] = unmasked_data;

        if (ws_frame.mask_idx != 0 && (ws_frame.mask_idx % 3) == 0)
        {
            ws_frame.mask_idx = -1;
        }
        ws_frame.mask_idx ++;
    }

    return 0;
}


int send_websocket_frame(struct client_data *client, const char *payload, size_t payload_len) {

    unsigned char frame_buf[BUFFER_SIZE];
    frame_buf[0] = 129;
    size_t offset;

    if (payload_len < 126)
    {
        offset = 2;
        frame_buf[1] = (char)payload_len;
    }
    else if (payload_len > 126 && payload_len < (BUFFER_SIZE - 4))
    {
        offset = 4;
        frame_buf[2] = (payload_len & 65280);
        frame_buf[3] = (payload_len & 255);
    }
    else 
    {
        printf("payload length: %ld is not supported\n", payload_len);
        return 1;
    }

    memcpy(frame_buf + offset, payload, payload_len);

    size_t msg_size = offset + payload_len;

    write(client->sockfd, frame_buf, msg_size);

    return 0;
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
