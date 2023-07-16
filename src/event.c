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
                return 1; // TODO return "NOTICE" event with debug message
            }
            printf("DEBUG json curly brace starting position: %d\n", event_json_start);

            if (find_json_end_position(event.raw_event_data, event_json_start, &event_json_end) != 0)
            {
                return 1; // TODO return "NOTICE" event with debug message
            }
            printf("DEBUG json curly brace ending position: %d\n", event_json_end);

            event_json = malloc((event_json_end - event_json_start + 2));
            event_json[sizeof(event_json)] = '\0';

            for (int i = 0; i < (event_json_end - event_json_start + 1); i++)
            {
                event_json[i] = event.raw_event_data[event_json_start + i];
            }

            if (validate_event_json(event_json) != 0)
            {
                return 1; // TODO return "NOTICE" event with debug message
            }

            free(event_json);

            break;
    }

    free(event_type);
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


int validate_event_json(char *event_json)
{
    struct event_elements
    {
        char *key_string;
        int key_string_pos;
        int key_string_valid;
    };

    struct event_elements id;
    struct event_elements pubkey;
    struct event_elements created_at;
    struct event_elements kind;
    struct event_elements tags;
    struct event_elements content;
    struct event_elements sig;


    char *id_string = "\"id\":";
    char *pubkey_string = "\"pubkey\":";
    char *created_at_string = "\"created_at\":";
    char *kind_string = "\"kind\":";
    char *tags_string = "\"tags\":";
    char *content_string = "\"content\":";
    char *sig_string = "\"sig\":";
    printf("DEBUG validating event json: %s\n", event_json);

    
    return 0;
}
