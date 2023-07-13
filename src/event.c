#include "../include/event.h"

int validate_nostr_event(struct nostr_event event)
{
    int event_type_kw_start;
    int event_json_start;
    int event_sub_id_start;
    int event_filters_start;
    int count;
    char *event_type;
    /*
    Clients can send 3 types of messages, which must be JSON arrays, according to the following patterns:

    ["EVENT", <event JSON as defined above>], used to publish events.
    ["REQ", <subscription_id>, <filters JSON>...], used to request events and subscribe to new updates.
    ["CLOSE", <subscription_id>], used to stop previous subscriptions.

    <subscription_id> is an arbitrary, non-empty string of max length 64 chars, that should be used to represent a subscription.

    <filters> is a JSON object that determines what events will be sent in that subscription, it can have the following attributes:

    TODO:
    for EVENT type check if all the data is inside {}, check if all of the required data is present and in the correct format, validate the event signature;

    */
    // Check if event data starts and ends with brackets [ and ]

    
    char debug1 = event.raw_event_data[0];
    char debug2 = event.raw_event_data[event.event_size];
    printf("DEBUG opening/closing brackets: opening is - %c, closing is %c\n", event.raw_event_data[0], event.raw_event_data[event.event_size - 1]);
    if (event.raw_event_data[0] != '[' ||
            event.raw_event_data[event.event_size - 1] != ']')
    {
        printf("DEBUG error at bracket check\n");
        return 1; // TODO return "NOTICE" event with debug message
    }
    // Check if event includes the correct keywords, keywords must be in double quotes
    event_type_kw_start = 2;

    char debug = event.raw_event_data[event_type_kw_start - 1];
    printf("DEBUG opening parantheses of event type: %c\n", debug);
    if (event.raw_event_data[event_type_kw_start - 1] != '"')
    {
        printf("DEBUG error at opening parantheses check\n");
        return 1; // TODO return "NOTICE" event with debug message
    }

    // First get the length of event type keyword
    count = 0;
    while(event.raw_event_data[event_type_kw_start + count] != '"')
    {
        count++;
    }

    // Next allocate memory for keyword and copy it it into the variable
    event_type = malloc(count + 1);
    for (int i = 0; i < count; i++)
    {
        event_type[i] = event.raw_event_data[event_type_kw_start + i];
    }
    event_type[count] = '\0';
    printf("DEBUG: %s\n", event_type);

    return 0;
}

