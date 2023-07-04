#include "../include/crypto.h"

int verify_event_data(struct event_to_verify *event)
{
    unsigned char public_key[32];
    unsigned char message_hash[32];
    unsigned char signature[32];

    // Convert hexadecimal strings to binary data
    hex_to_bin(event->public_key_hex, public_key, sizeof(public_key));
    hex_to_bin(event->hashed_message_hex, message_hash, sizeof(message_hash));
    hex_to_bin(event->signature_hex, signature, sizeof(signature));

    // Create a context
    secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);

    // Parse the public key
    secp256k1_xonly_pubkey pubkey;
    if (!secp256k1_xonly_pubkey_parse(ctx, &pubkey, public_key))
    {
        printf("Failed parsing the public key\n");
        return 1;
    }

    // Verify the signature
    event->is_signature_valid = secp256k1_schnorrsig_verify(ctx, signature, message_hash, sizeof(message_hash), &pubkey);

    // Destrtoy the context
    secp256k1_context_destroy(ctx);

    return 0;
}
