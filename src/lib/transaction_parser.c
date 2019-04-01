/*******************************************************************************
*   (c) 2018 ZondaX GmbH
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/

#include <jsmn.h>
#include <stdio.h>
#include "transaction_parser.h"
#include "json_parser.h"

//---------------------------------------------

const char whitespaces[] = {
        0x20,// space ' '
        0x0c, // form_feed '\f'
        0x0a, // line_feed, '\n'
        0x0d, // carriage_return, '\r'
        0x09, // horizontal_tab, '\t'
        0x0b // vertical_tab, '\v'
};

//---------------------------------------------

int msgs_total_pages = 0;
int msgs_array_elements = 0;

//---------------------------------------------

copy_delegate copy_fct = NULL;
parsing_context_t parsing_context;

void set_copy_delegate(copy_delegate delegate) {
    copy_fct = delegate;
}

void set_parsing_context(parsing_context_t context) {
    parsing_context = context;
}

//--------------------------------------
// Transaction parsing helper functions
//--------------------------------------
void update(char *msg,
            int16_t msg_length,
            int16_t token_index,
            int16_t *chunk_index) {
    int length = parsing_context.parsed_transaction->Tokens[token_index].end -
                 parsing_context.parsed_transaction->Tokens[token_index].start;

    int chunk_to_display = *chunk_index;
    *chunk_index = (length / msg_length) + 1;

    if (chunk_to_display >= 0 && chunk_to_display < *chunk_index) {
        length = parsing_context.parsed_transaction->Tokens[token_index].end -
                 parsing_context.parsed_transaction->Tokens[token_index].start - (msg_length - 1) * chunk_to_display;

        if (length + 1 > msg_length) {
            // Return total number of chunks
            length = msg_length - 1;
        }
        copy_fct(msg,
                 parsing_context.transaction + parsing_context.parsed_transaction->Tokens[token_index].start +
                 (msg_length - 1) * chunk_to_display,
                 length);
        msg[length] = '\0';
    } else {
        msg[0] = '\0';
    }
}

int display_value(char *value,
                  int16_t value_length,
                  int16_t token_index,
                  int16_t *current_item_index,
                  int16_t item_index_to_display,
                  int16_t *chunk_index) {

    if (*current_item_index == item_index_to_display) {
        update(value, value_length, token_index, chunk_index);
        return item_index_to_display;
    }

    *current_item_index = *current_item_index + 1;
    return -1;
}

void display_key(
        char *key,
        int16_t key_length,
        int16_t token_index) {
    int key_size = parsing_context.parsed_transaction->Tokens[token_index].end
                   - parsing_context.parsed_transaction->Tokens[token_index].start;
    const char
            *address_ptr = parsing_context.transaction + parsing_context.parsed_transaction->Tokens[token_index].start;
    if (key_size >= key_length) {
        key_size = key_length - 1;
    }
    copy_fct(key, address_ptr, key_size);
    key[key_size] = '\0';
}

void append_keys(char *key, int16_t key_length, const char *temp_key) {
    int size = strlen(key);

    if (size > 0) {
        key[size] = '/';
        size++;
    }

    strcpy(key + size, temp_key);
}

void remove_last(char *key) {
    size_t size = strlen(key);
    char *last = key + size;
    while (last > key) {
        if (*last == '/') {
            *last = '\0';
            return;
        }
        last--;
    }
    *last = '\0';
}

// in order to reduce stack size usage
typedef struct {
    int16_t item_index_to_display;
    char *key;
    char *value;
    int16_t key_length;
    int16_t value_length;
} display_arbitrary_params_t;


int display_arbitrary_item_inner(display_arbitrary_params_t *p,
                                 int16_t token_index, int16_t *current_item_index,
                                 uint8_t level, int16_t *chunk_index,
                                 uint8_t depth) {

//    if level == 2
//    show value as json-encoded string
//    else
//    switch typeof(json) {
//        case object:
//            for (key, value) in object:
//                    show key
//                    display(value, level + 1)
//        case array:
//            for element in array:
//        display(element, level + 1)
//        otherwise:
//        show value as json-encoded string
//    }

    if (level >= 2 || depth >= 4) {
        return display_value(p->value, p->value_length,
                             token_index, current_item_index,
                             p->item_index_to_display, chunk_index);
    }

    switch (parsing_context.parsed_transaction->Tokens[token_index].type) {
        case JSMN_STRING:
        case JSMN_PRIMITIVE:
            return display_value(
                    p->value,
                    p->value_length,
                    token_index,
                    current_item_index,
                    p->item_index_to_display,
                    chunk_index);

        case JSMN_OBJECT: {
            int el_count = object_get_element_count(token_index, parsing_context.parsed_transaction);
            for (int i = 0; i < el_count; ++i) {
                int key_index = object_get_nth_key(token_index, i, parsing_context.parsed_transaction);
                int value_index = object_get_nth_value(token_index, i, parsing_context.parsed_transaction);

                if (p->item_index_to_display != -1) {
                    char key_temp[20];
                    display_key(
                            key_temp,
                            sizeof(key_temp),
                            key_index);

                    append_keys(p->key, p->key_length, key_temp);
                }

                int found = display_arbitrary_item_inner(
                        p,
                        value_index,
                        current_item_index,
                        level + 1,
                        chunk_index,
                        depth + 1);

                if (p->item_index_to_display != -1) {
                    if (found == p->item_index_to_display) {
                        return p->item_index_to_display;
                    } else {
                        remove_last(p->key);
                    }
                }
            }
            break;
        }
        case JSMN_ARRAY: {
            int el_count = array_get_element_count(token_index, parsing_context.parsed_transaction);

            for (int i = 0; i < el_count; ++i) {
                int element_index = array_get_nth_element(token_index, i, parsing_context.parsed_transaction);
                int found = display_arbitrary_item_inner(p,
                                                         element_index, current_item_index,
                                                         level, chunk_index, depth + 1);

                if (p->item_index_to_display != -1) {
                    if (found == p->item_index_to_display) {
                        return p->item_index_to_display;
                    }
                }
            }
            break;
        }
        default:
            return *current_item_index;
    }
    // Not found yet, continue parsing
    return -1;
}

int display_get_arbitrary_items_count(int16_t token_index) {
    display_arbitrary_params_t params;

    int16_t number_of_items = 0;
    int16_t chunk_index = 0;
    char dummy[1];

    params.item_index_to_display = -1;
    params.key = dummy;
    params.value = dummy;
    params.key_length = 1;
    params.value_length = 1;

    const int8_t start_level = 0;
    const int8_t start_depth = 0;

    display_arbitrary_item_inner(&params,
                                 token_index, &number_of_items,
                                 start_level, &chunk_index, start_depth);

    return number_of_items;
}

int display_arbitrary_item(int16_t item_index_to_display,
                           char *key, int16_t key_length,
                           char *value, int16_t value_length,
                           int16_t token_index, int16_t *chunk_index) {

    display_arbitrary_params_t params;
    params.item_index_to_display = item_index_to_display;
    params.key = key;
    params.value = value;
    params.key_length = key_length;
    params.value_length = value_length;

    int16_t current_item_index = 0;

    const int8_t start_level = 0;
    const int8_t start_depth = 0;
    return display_arbitrary_item_inner(&params,
                                        token_index, &current_item_index,
                                        start_level, chunk_index, start_depth);
}

int transaction_get_display_key_value(char *key, int16_t max_key_length,
                                      char *value, int16_t max_value_length,
                                      int16_t page_index, int16_t *chunk_index) {
    const int non_msg_pages_count = 5;
    if (page_index >= 0 && page_index < non_msg_pages_count) {
        const char *key_name;
        switch (page_index) {
            case 0:
                key_name = "chain_id";
                break;
            case 1:
                key_name = "account_number";
                break;
            case 2:
                key_name = "sequence";
                break;
            case 3:
                key_name = "fee";
                break;
            case 4:
                key_name = "memo";
                break;
            default:
                key_name = "????";
        }

        strcpy(key, key_name);
        int token_index = object_get_value(ROOT_TOKEN_INDEX,
                                           key_name,
                                           parsing_context.parsed_transaction,
                                           parsing_context.transaction);
        update(value, max_value_length, token_index, chunk_index);
    } else {
        int msgs_page_to_display = page_index - non_msg_pages_count;
        int subpage_to_display = msgs_page_to_display;
        if (msgs_page_to_display < msgs_total_pages) {
            int msgs_array_token_index = object_get_value(
                    ROOT_TOKEN_INDEX,
                    "msgs",
                    parsing_context.parsed_transaction,
                    parsing_context.transaction);

            int total = 0;
            int msgs_array_index = 0;
            int msgs_token_index = 0;
            for (int i = 0; i < msgs_array_elements; i++) {
                int token_index_of_msg = array_get_nth_element(msgs_array_token_index, i,
                                                               parsing_context.parsed_transaction);
                int count = display_get_arbitrary_items_count(token_index_of_msg);
                total += count;
                if (msgs_page_to_display < total) {
                    msgs_token_index = token_index_of_msg;
                    msgs_array_index = i;
                    break;
                }
                subpage_to_display -= count;
            }

            snprintf(key, max_key_length, "msgs_%d", msgs_array_index + 1);

            display_arbitrary_item(subpage_to_display,
                                   key,
                                   max_key_length,
                                   value,
                                   max_value_length,
                                   msgs_token_index,
                                   chunk_index);
        }
    }
    return 0;
}

int transaction_get_display_pages() {
    int msgs_token_index = object_get_value(
            ROOT_TOKEN_INDEX,
            "msgs",
            parsing_context.parsed_transaction,
            parsing_context.transaction);

    msgs_array_elements = array_get_element_count(msgs_token_index, parsing_context.parsed_transaction);

    msgs_total_pages = 0;
    for (int i = 0; i < msgs_array_elements; i++) {
        int token_index_of_msg = array_get_nth_element(msgs_token_index, i, parsing_context.parsed_transaction);
        msgs_total_pages += display_get_arbitrary_items_count(token_index_of_msg);
    }
    return msgs_total_pages + 5;
}

int is_space(char c) {
    for (unsigned int i = 0; i < sizeof(whitespaces); i++) {
        if (whitespaces[i] == c) {
            return 1;
        }
    }
    return 0;
}

int contains_whitespace(parsed_json_t *parsed_transaction,
                        const char *transaction) {

    int start = 0;
    int last_element_index = parsed_transaction->Tokens[0].end;

    // Starting at token 1 because token 0 contains full transaction
    for (int i = 1; i < parsed_transaction->NumberOfTokens; i++) {
        if (parsed_transaction->Tokens[i].type != JSMN_UNDEFINED) {
            int end = parsed_transaction->Tokens[i].start;
            for (int j = start; j < end; j++) {
                if (is_space(transaction[j]) == 1) {
                    return 1;
                }
            }
            start = parsed_transaction->Tokens[i].end + 1;
        } else {
            return 0;
        }
    }
    while (start <= last_element_index && transaction[start] != '\0') {
        if (is_space(transaction[start]) == 1) {
            return 1;
        }
        start++;
    }
    return 0;
}

int is_sorted(int16_t first_index,
              int16_t second_index,
              parsed_json_t *parsed_transaction,
              const char *transaction) {
#if DEBUG_SORTING
    char first[256];
    char second[256];

    int size =  parsed_transaction->Tokens[first_index].end - parsed_transaction->Tokens[first_index].start;
    strncpy(first, transaction + parsed_transaction->Tokens[first_index].start, size);
    first[size] = '\0';
    size =  parsed_transaction->Tokens[second_index].end - parsed_transaction->Tokens[second_index].start;
    strncpy(second, transaction + parsed_transaction->Tokens[second_index].start, size);
    second[size] = '\0';
#endif

    if (strcmp(
            transaction + parsed_transaction->Tokens[first_index].start,
            transaction + parsed_transaction->Tokens[second_index].start) <= 0) {
        return 1;
    }
    return 0;
}

int dictionaries_sorted(parsed_json_t *parsed_transaction,
                        const char *transaction) {
    for (int i = 0; i < parsed_transaction->NumberOfTokens; i++) {
        if (parsed_transaction->Tokens[i].type == JSMN_OBJECT) {

            int count = object_get_element_count(i, parsed_transaction);
            if (count > 1) {
                int prev_token_index = object_get_nth_key(i, 0, parsed_transaction);
                for (int j = 1; j < count; j++) {
                    int next_token_index = object_get_nth_key(i, j, parsed_transaction);
                    if (!is_sorted(prev_token_index, next_token_index, parsed_transaction, transaction)) {
                        return 0;
                    }
                    prev_token_index = next_token_index;
                }
            }
        }
    }
    return 1;
}

const char *json_validate(parsed_json_t *parsed_transaction,
                          const char *transaction) {

    if (contains_whitespace(parsed_transaction, transaction) == 1) {
        return "JSON Contains whitespace in the corpus";
    }

    if (dictionaries_sorted(parsed_transaction, transaction) != 1) {
        return "JSON Dictionaries are not sorted";
    }

    if (object_get_value(0,
                         "chain_id",
                         parsed_transaction,
                         transaction) == -1) {
        return "JSON Missing chain_id";
    }

    if (object_get_value(0,
                         "sequence",
                         parsed_transaction,
                         transaction) == -1) {
        return "JSON Missing sequence";
    }

    if (object_get_value(0,
                         "fee",
                         parsed_transaction,
                         transaction) == -1) {
        return "JSON Missing fee";
    }

    if (object_get_value(0,
                         "msgs",
                         parsed_transaction,
                         transaction) == -1) {
        return "JSON Missing msgs";
    }

    if (object_get_value(0,
                         "account_number",
                         parsed_transaction,
                         transaction) == -1) {
        return "JSON Missing account_number";
    }

    if (object_get_value(0,
                         "memo",
                         parsed_transaction,
                         transaction) == -1) {
        return "JSON Missing memo";
    }

    return NULL;
}