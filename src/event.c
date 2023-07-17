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
    const char *id_key = "\"id\":";
    const char *pubkey_key = "\"pubkey\":";
    const char *created_at_key = "\"created_at\":";
    const char *kind_key = "\"kind\":";
    const char *tags_key = "\"tags\":";
    const char *content_key = "\"content\":";
    const char *sig_key = "\"sig\":";

    int id_key_pos;
    int pubkey_key_pos;
    int created_at_key_pos;
    int kind_key_pos;
    int tags_key_pos;
    int content_key_pos;
    int sig_key_pos;

    int id_value_start_pos;
    int pubkey_start_key_pos;
    int created_at_value_start_pos;
    int kind_value_start_pos;
    int tags_value_start_pos;
    int content_value_start_pos;
    int sig_value_start_pos;

    const int id_value_len = 32;
    const int pubkey_value_len = 32;
    const int sig_value_len = 64;

    printf("DEBUG validating event json: %s\n", event_json);

    if ((id_key_pos = get_substring_pos(event_json, (char *)id_key)) == 0)
    {
        printf("DEBUG error: id key not found in event json\n");
        return 1; // TODO return "NOTICE" event with debug message
    }
    printf("DEBUG beginning position of 'id' key in event json is: %d\n", id_key_pos);

    if ((pubkey_key_pos = get_substring_pos(event_json, (char *)pubkey_key)) == 0)
    {
        printf("DEBUG error: pubkey key not found in event json\n");
        return 1; // TODO return "NOTICE" event with debug message
    }
    printf("DEBUG beginning position of 'pubkey' key in event json is: %d\n", pubkey_key_pos);

    if ((created_at_key_pos = get_substring_pos(event_json, (char *)created_at_key)) == 0)
    {
        printf("DEBUG error: created_at key not found in event json\n");
        return 1; // TODO return "NOTICE" event with debug message
    }
    printf("DEBUG beginning position of 'created_at' key in event json is: %d\n", created_at_key_pos);

    if ((kind_key_pos = get_substring_pos(event_json, (char *)kind_key)) == 0)
    {
        printf("DEBUG error: kind key not found in event json\n");
        return 1; // TODO return "NOTICE" event with debug message
    }
    printf("DEBUG beginning position of 'kind' key in event json is: %d\n", kind_key_pos);

    if ((tags_key_pos = get_substring_pos(event_json, (char *)tags_key)) == 0)
    {
        return 1; // TODO return "NOTICE" event with debug message
        printf("DEBUG error: tags key not found in event json\n");
    }
    printf("DEBUG beginning position of 'tags' key in event json is: %d\n", tags_key_pos);

    if ((content_key_pos = get_substring_pos(event_json, (char *)content_key)) == 0)
    {
        printf("DEBUG error: content key not found in event json\n");
        return 1; // TODO return "NOTICE" event with debug message
    }
    printf("DEBUG beginning position of 'content' key in event json is: %d\n", content_key_pos);

    if ((sig_key_pos = get_substring_pos(event_json, (char *)sig_key)) == 0)
    {
        printf("DEBUG error: sig key not found in event json\n");
        return 1; // TODO return "NOTICE" event with debug message
    }
    printf("DEBUG beginning position of 'sig' key in event json is: %d\n", sig_key_pos);

    return 0;
}
