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
#include "json_parser.h"

int msg_bytes_pages = 0;
int alt_bytes_pages = 0;
//---------------------------------------------

copy_delegate copy_fct = NULL;
parsing_context_t parsing_context;

void set_copy_delegate(copy_delegate delegate) {
    copy_fct = delegate;
}

void set_parsing_context(parsing_context_t context) {
    parsing_context = context;
}

//---------------------------------------------

void json_parse(parsed_json_t *parsed_json, const char *transaction) {
    jsmn_parser parser;
    jsmn_init(&parser);

    int num_tokens = jsmn_parse(
            &parser,
            transaction,
            strlen(transaction),
            parsed_json->Tokens,
            MAX_NUMBER_OF_TOKENS);

    parsed_json->CorrectFormat = false;

    if (num_tokens > 255) {
        return;
    }

    parsed_json->NumberOfTokens = (byte) num_tokens;
    if (parsed_json->NumberOfTokens >= 1 &&
        parsed_json->Tokens[0].type != JSMN_OBJECT) {
        parsed_json->CorrectFormat = true;
    }
}

int json_validate(const char *transaction,
                  char *errorMsg,
                  int errMsgLength) {
    strcpy(errorMsg, "Not implemented.");
    // Here we make sure that json have correct format according to the spec
    return -1;
}

int array_get_element_count(int array_token_index,
                            const parsed_json_t *parsed_transaction) {
    jsmntok_t array_token = parsed_transaction->Tokens[array_token_index];
    int token_index = array_token_index;
    int element_count = 0;
    int prev_element_end = array_token.start;
    while (true) {
        token_index++;
        if (token_index >= parsed_transaction->NumberOfTokens) {
            break;
        }
        jsmntok_t current_token = parsed_transaction->Tokens[token_index];
        if (current_token.start > array_token.end) {
            break;
        }
        if (current_token.start <= prev_element_end) {
            continue;
        }
        prev_element_end = current_token.end;
        element_count++;
    }

    return element_count;
}

int array_get_nth_element(int array_token_index,
                          int element_index,
                          const parsed_json_t *parsed_transaction) {
    jsmntok_t array_token = parsed_transaction->Tokens[array_token_index];
    int token_index = array_token_index;
    int element_count = 0;
    int prev_element_end = array_token.start;
    while (true) {
        token_index++;
        if (token_index >= parsed_transaction->NumberOfTokens) {
            break;
        }
        jsmntok_t current_token = parsed_transaction->Tokens[token_index];
        if (current_token.start > array_token.end) {
            break;
        }
        if (current_token.start <= prev_element_end) {
            continue;
        }
        prev_element_end = current_token.end;
        if (element_count == element_index) {
            return token_index;
        }
        element_count++;
    }

    return -1;
}

int object_get_element_count(int object_token_index,
                             const parsed_json_t *parsed_transaction) {
    jsmntok_t object_token = parsed_transaction->Tokens[object_token_index];
    int token_index = object_token_index;
    int element_count = 0;
    int prev_element_end = object_token.start;
    token_index++;
    while (true) {
        if (token_index >= parsed_transaction->NumberOfTokens) {
            break;
        }
        jsmntok_t key_token = parsed_transaction->Tokens[token_index++];
        jsmntok_t value_token = parsed_transaction->Tokens[token_index];
        if (key_token.start > object_token.end) {
            break;
        }
        if (key_token.start <= prev_element_end) {
            continue;
        }
        prev_element_end = value_token.end;
        element_count++;
    }

    return element_count;
}

int object_get_nth_key(int object_token_index,
                       int object_element_index,
                       const parsed_json_t *parsed_transaction) {
    jsmntok_t object_token = parsed_transaction->Tokens[object_token_index];
    int token_index = object_token_index;
    int element_count = 0;
    int prev_element_end = object_token.start;
    token_index++;
    while (true) {
        if (token_index >= parsed_transaction->NumberOfTokens) {
            break;
        }
        jsmntok_t key_token = parsed_transaction->Tokens[token_index++];
        jsmntok_t value_token = parsed_transaction->Tokens[token_index];
        if (key_token.start > object_token.end) {
            break;
        }
        if (key_token.start <= prev_element_end) {
            continue;
        }
        prev_element_end = value_token.end;
        if (element_count == object_element_index) {
            return token_index - 1;
        }
        element_count++;
    }

    return -1;
}

int object_get_nth_value(int object_token_index,
                         int object_element_index,
                         const parsed_json_t *parsed_transaction) {
    int key_index = object_get_nth_key(object_token_index, object_element_index, parsed_transaction);
    if (key_index != -1) {
        return key_index + 1;
    }
    return -1;
}

int object_get_value(int object_token_index,
                     const char *key_name,
                     const parsed_json_t *parsed_transaction,
                     const char *transaction) {
    size_t length = strlen(key_name);
    jsmntok_t object_token = parsed_transaction->Tokens[object_token_index];
    int token_index = object_token_index;
    int prev_element_end = object_token.start;
    token_index++;
    while (true) {
        if (token_index >= parsed_transaction->NumberOfTokens) {
            break;
        }
        jsmntok_t key_token = parsed_transaction->Tokens[token_index++];
        jsmntok_t value_token = parsed_transaction->Tokens[token_index];
        if (key_token.start > object_token.end) {
            break;
        }
        if (key_token.start <= prev_element_end) {
            continue;
        }
        prev_element_end = value_token.end;
        char *cmper = (char *) (transaction + key_token.start);
        if (memcmp(key_name, cmper, length) == 0) {
            return token_index;
        }
    }

    return -1;
}

//--------------------------------------
// TODO: Move to a separate file
// Transaction parsing helper functions
//--------------------------------------
void update(char *msg,// output
            int msg_length,
            int token_index,
            int *chunk_index) // input
{
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
                  int value_length,
                  int token_index,
                  int *current_item_index,
                  int item_index_to_display,
                  int *chunk_index) {

    if (*current_item_index == item_index_to_display) {

        update(value, value_length, token_index, chunk_index);

        return item_index_to_display;
    }
    *current_item_index = *current_item_index + 1;
    return -1;
}

void display_key(
        char* key,
        int key_length,
        int token_index)
{
    int key_size = parsing_context.parsed_transaction->Tokens[token_index].end - parsing_context.parsed_transaction->Tokens[token_index].start;
    const char* address_ptr = parsing_context.transaction + parsing_context.parsed_transaction->Tokens[token_index].start;
    if (key_size >= key_length) {
        key_size = key_length-1;
    }
    copy_fct(key, address_ptr, key_size);
    key[key_size] = '\0';
}

void append_keys(char* key, int key_length, const char* temp_key)
{
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

int display_arbitrary_item_inner(
        int item_index_to_display, //input
        char* key, // output
        int key_length,
        char* value, // output
        int value_length,
        int token_index, // input
        int* current_item_index, // input
        int level,
        int* chunk_index)
{
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
    if (level == 2) {
        return display_value(
                value,
                value_length,
                token_index,
                current_item_index,
                item_index_to_display,
                chunk_index);
    } else {
        switch (parsing_context.parsed_transaction->Tokens[token_index].type) {
            case JSMN_STRING:
                return display_value(
                        value,
                        value_length,
                        token_index,
                        current_item_index,
                        item_index_to_display,
                        chunk_index);

            case JSMN_PRIMITIVE:
                return display_value(
                        value,
                        value_length,
                        token_index,
                        current_item_index,
                        item_index_to_display,
                        chunk_index);

            case JSMN_OBJECT: {
                int el_count = object_get_element_count(token_index, parsing_context.parsed_transaction);
                for (int i = 0; i < el_count; ++i) {
                    int key_index = object_get_nth_key(token_index, i, parsing_context.parsed_transaction);
                    int value_index = object_get_nth_value(token_index, i, parsing_context.parsed_transaction);

                    if (item_index_to_display != -1) {
                        char key_temp[20];
                        display_key(
                                key_temp,
                                sizeof(key_temp),
                                key_index);

                        append_keys(key, key_length, key_temp);
                    }

                    int found = display_arbitrary_item_inner(
                            item_index_to_display,
                            key,
                            key_length,
                            value,
                            value_length,
                            value_index,
                            current_item_index,
                            level + 1,
                            chunk_index);

                    if (item_index_to_display != -1) {
                        if (found == item_index_to_display) {
                            return item_index_to_display;
                        } else {
                            remove_last(key);
                        }
                    }
                }
                break;
            }
            case JSMN_ARRAY: {
                int el_count = array_get_element_count(token_index, parsing_context.parsed_transaction);
                for (int i = 0; i < el_count; ++i) {
                    int element_index = array_get_nth_element(token_index, i, parsing_context.parsed_transaction);
                    int found = display_arbitrary_item_inner(
                            item_index_to_display,
                            key,
                            key_length,
                            value,
                            value_length,
                            element_index,
                            current_item_index,
                            level,
                            chunk_index);

                    if (item_index_to_display != -1) {
                        if (found == item_index_to_display) {
                            return item_index_to_display;
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
}

int display_get_arbitrary_items_count(int token_index) {
    int number_of_items = 0;
    int chunk_index = 0;
    char dummy[1];
    display_arbitrary_item_inner(
            -1,
            dummy,
            1,
            dummy,
            1,
            token_index,
            &number_of_items,
            0,
            &chunk_index);

    return number_of_items;
}

int display_arbitrary_item(
        int item_index_to_display, //input
        char* key, // output
        int key_length,
        char* value, // output
        int value_length,
        int token_index,
        int* chunk_index)
{
    int current_item_index = 0;
    return display_arbitrary_item_inner(
            item_index_to_display,
            key,
            key_length,
            value,
            value_length,
            token_index,
            &current_item_index,
            0,
            chunk_index);
}

int transaction_get_display_key_value(
        char* key, // output
        int key_length,
        char* value, // output
        int value_length,
        int page_index,
        int* chunk_index) // input/output
{
    switch (page_index) {
        case 0: {
            copy_fct(key, "chain_id", sizeof("chain_id"));
            int token_index = object_get_value(0, "chain_id", parsing_context.parsed_transaction,
                                               parsing_context.transaction);
            update(value, value_length, token_index, chunk_index);
            break;
        }
        case 1: {
            copy_fct(key, "sequence", sizeof("sequence"));
            int token_index = object_get_value(0, "sequence", parsing_context.parsed_transaction,
                                               parsing_context.transaction);
            update(value, value_length, token_index, chunk_index);
            break;
        }
        case 2: {
            copy_fct(key, "fee_bytes", sizeof("fee_bytes"));
            int token_index = object_get_value(0, "fee_bytes", parsing_context.parsed_transaction,
                                               parsing_context.transaction);
            update(value, value_length, token_index, chunk_index);
            break;
        }
        default: {
            if (page_index - 3 < msg_bytes_pages) {
                int token_index = object_get_value(0,
                                                   "msg_bytes",
                                                   parsing_context.parsed_transaction,
                                                   parsing_context.transaction);

                //char full_key[parsing_context.max_chars_per_key_line];
                copy_fct(key, "msg_bytes", sizeof("msg_bytes"));
                key[sizeof("msg_bytes")] = '\0';

                display_arbitrary_item(page_index - 3,
                                       key,
                                       key_length,
                                       value,
                                       value_length,
                                       token_index,
                                       chunk_index);
            }
            else {
                int token_index = object_get_value(0, "alt_bytes", parsing_context.parsed_transaction,
                                                   parsing_context.transaction);
                //char full_key[parsing_context.max_chars_per_key_line];
                copy_fct(key, "alt_bytes", sizeof("alt_bytes"));
                key[sizeof("alt_bytes")] = '\0';

                display_arbitrary_item(page_index - 3 - msg_bytes_pages,
                                       key,
                                       key_length,
                                       value,
                                       value_length,
                                       token_index,
                                       chunk_index);
            }
            break;
        }
    }
    return 0;
}

int transaction_get_display_pages() {
    int token_index_mb = object_get_value(0, "msg_bytes", parsing_context.parsed_transaction,
                                          parsing_context.transaction);
    int token_index_ab = object_get_value(0, "alt_bytes", parsing_context.parsed_transaction,
                                          parsing_context.transaction);

    msg_bytes_pages = display_get_arbitrary_items_count(token_index_mb);
    alt_bytes_pages = display_get_arbitrary_items_count(token_index_ab);

    return msg_bytes_pages + alt_bytes_pages + 3;
}
