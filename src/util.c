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


