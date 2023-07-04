#include "../include/util.h"



void base64_encode(const unsigned char *input, size_t input_length, char *output)
{
    EVP_ENCODE_CTX *ctx = EVP_ENCODE_CTX_new();
    int output_length;
    EVP_EncodeInit(ctx);
    EVP_EncodeUpdate(ctx, (unsigned char *)output, &output_length, input, input_length);
    EVP_EncodeFinal(ctx, (unsigned char *)&output[output_length], &output_length);
    EVP_ENCODE_CTX_free(ctx);
}

int hex_to_bin(const char* hex_string, unsigned char* bin_data, size_t bin_size)
{
    size_t hex_len = strlen(hex_string);
    if (hex_len % 2 != 0 || hex_len / 2 != bin_size)
    {
        return 1; // Invalid input size
    }

    for (size_t i = 0; i < hex_len; i += 2)
    {
        int parsed_value;
        if (sscanf(&hex_string[i], "%2x", &parsed_value) != 1)
        {
            return 1; // Parsing failed
        }
        bin_data[i / 2] = (unsigned char)parsed_value;
    }

    return 0; // Success
}
