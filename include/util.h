#ifndef UTIL
#define UTIL

#include <openssl/sha.h>
#include <openssl/evp.h>
#include <string.h>
#include <stdio.h>

void base64_encode(const unsigned char *input, size_t input_length, char *output);
int hex_to_bin(const char* hex_string, unsigned char* bin_data, size_t bin_size);

#endif
