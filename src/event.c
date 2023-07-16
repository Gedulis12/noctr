#include "../include/event.h"

int validate_nostr_event(struct nostr_event event)
{
    uint event_type_kw_start = 2;
    int event_json_start;
    int event_json_end;
    char *event_json;
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
    //
    // Check if event data starts and ends with brackets [ and ]
    printf("DEBUG opening/closing brackets: opening is - %c, closing is %c\n", event.raw_event_data[0], event.raw_event_data[event.event_size - 1]);
    if (event.raw_event_data[0] != '[' ||
            event.raw_event_data[event.event_size - 1] != ']')
    {
        printf("DEBUG error at bracket check\n");
        return 1; // TODO return "NOTICE" event with debug message
    }

    // Check if event includes the correct keywords, keywords must be in double quotes

    char debug = event.raw_event_data[event_type_kw_start - 1];
    printf("DEBUG opening parantheses of event type: %c\n", debug);
    if (event.raw_event_data[event_type_kw_start - 1] != '"')
    {
        printf("DEBUG error at opening parantheses check\n");
        return 1; // TODO return "NOTICE" event with debug message
    }

    // Get the length of event type keyword
    count = 0;
    while(event.raw_event_data[event_type_kw_start + count] != '"')
    {
        count++;
        if (count == strlen(event.raw_event_data))
        {
            return 1; // TODO return "NOTICE" event with debug message
        }
    }

    // Allocate memory for keyword and copy it it into the variable
    event_type = malloc(count + 1);
    for (int i = 0; i < count; i++)
    {
        event_type[i] = event.raw_event_data[event_type_kw_start + i];
    }
    event_type[count] = '\0';
    printf("DEBUG: %s\n", event_type);

    // Assign the event type to struct
    if (strcmp(event_type, "EVENT") == 0)
    {
        event.event_type = 1;
    }
    else if (strcmp(event_type, "REQ") == 0)
    {
        event.event_type = 2;
    }
    else if (strcmp(event_type, "CLOSE") == 0)
    {
        event.event_type = 3;
    }
    else
    {
        printf("DEBUG error at event type check\n");
        return 1; // TODO return "NOTICE" event with debug message
    }

    // Check if the comma is present after the event type
    if (event.raw_event_data[strlen(event_type) + event_type_kw_start + 1] != ',')
    {
        printf("DEBUG error missing comma after the event type, found: %c\n",event.raw_event_data[strlen(event_type) + event_type_kw_start + 1]);
        return 1; // TODO return "NOTICE" event with debug message
    }

    switch (event.event_type)
    {
        case 1:
            if (find_json_start_position(event.raw_event_data, &event_json_start) != 0)
            {
                return 1;
            }
            printf("DEBUG json curly brace starting position: %d\n", event_json_start);

            if (find_json_end_position(event.raw_event_data, event_json_start, &event_json_end) != 0)
            {
                return 1;
            }
            printf("DEBUG json curly brace ending position: %d\n", event_json_end);
            break;
    }

    return 0;
}


int find_json_start_position(char *event_raw_data, int *json_start_position)
{
    char *event_type_string = "\"EVENT\",";
    *json_start_position = 0;
    for (int i = 0; i < (strlen(event_raw_data) - strlen(event_type_string)); i++)
    {
        if (event_raw_data[strlen(event_type_string) + i] == '{')
        {
            *json_start_position = strlen(event_type_string) + i;
        }
    }

    if (*json_start_position < 1)
    {
        printf("DEBUG error missing opening curly brace for event\n");
        return 1; // TODO return "NOTICE" event with debug message
    }

    return 0;
}


int find_json_end_position(char *event_raw_data, int json_start_position, int *json_end_position)
{
    *json_end_position = 0;
    for (int i = strlen(event_raw_data); i > json_start_position; i--)
    {
        if (event_raw_data[i] == '}')
        {
            *json_end_position = i;
        }
    }

    if (*json_end_position < 1)
    {
        printf("DEBUG error missing closing curly brace for event\n");
        return 1; // TODO return "NOTICE" event with debug message
    }
    return 0;
}


int validate_event_json(struct nostr_event event)
{
    return 0;
}
