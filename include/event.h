#include <stddef.h>
#include <stdlib.h>

struct nostr_event
{
    size_t event_size;
    char *raw_event_data;
    unsigned char event_type; // 1 - "EVENT", 2 - "REQ", 3 - "CLOSE"
};

int validate_nostr_event(struct nostr_event);
