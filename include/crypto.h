#ifndef CRYPTO
#define CRYPTO

#include "util.h"
#include <secp256k1.h>
#include <secp256k1_extrakeys.h>
#include <secp256k1_schnorrsig.h>

struct event_to_verify
{
    const char public_key_hex[32];
    const char hashed_message_hex[32];
    const char signature_hex[64];
    int is_signature_valid;
};

#endif
