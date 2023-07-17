#ifndef EVENT
#define EVENT

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../include/util.h"

struct nostr_event
{
    size_t event_size;
    char *raw_event_data;
    uint event_type; // EVENT = 1, REQ = 2, CLOSE = 3
};

int validate_nostr_event(struct nostr_event);
int validate_event_json(char *event_json);
int find_json_start_position(char *event_raw_data, int *json_start_position);
int find_json_end_position(char *event_raw_data, int json_start_position, int *json_end_position);

#endif
