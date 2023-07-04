#include <openssl/sha.h>
#include <openssl/evp.h>
#include <string.h>
#include <stdio.h>

void base64_encode(const unsigned char *input, size_t input_length, char *output);
