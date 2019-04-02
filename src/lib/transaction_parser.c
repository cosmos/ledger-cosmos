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

#define MAX_RECURSION_DEPTH  5
#define MAX_TREE_LEVEL       5

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

int16_t msgs_total_pages = 0;
int16_t msgs_array_elements = 0;

//---------------------------------------------

copy_delegate copy_fct = NULL;          // Decoupled so we can handle both RAM and NVRAM with similar code
parsing_context_t parsing_context;

void set_copy_delegate(copy_delegate delegate) {
    copy_fct = delegate;
}

void set_parsing_context(parsing_context_t context) {
    parsing_context = context;
}

void strcat_s(char *dst, uint16_t dst_max, const char *src, uint16_t src_size) {
    char *p = dst;
    *(p + dst_max - 1) = 0;                 // last character terminates with zero

    const uint16_t prev_size = strlen(dst);
    p += prev_size;
    uint16_t space_left = dst_max - prev_size;

    if (src_size > space_left) {
        src_size = space_left - 1;
    }

    if (src_size > 0) {
        // Check bounds
        copy_fct(p, src, src_size);
    }

    // terminate
    *(p + src_size) = 0;
}

//--------------------------------------
// Transaction parsing helper functions
//--------------------------------------
int16_t update(char *out, const int16_t out_len, const int16_t token_index, uint16_t chunk_to_display) {

    const int16_t token_start = parsing_context.parsed_tx->Tokens[token_index].start;
    const int16_t token_end = parsing_context.parsed_tx->Tokens[token_index].end;
    const int16_t token_len = token_end - token_start;
    const int16_t num_chunks = (token_len / (out_len - 1)) + 1;

    out[0] = '\0';  // flush
    if (chunk_to_display < num_chunks) {
        const int16_t chunk_start = token_start + chunk_to_display * (out_len - 1);
        int16_t chunk_len = token_end - chunk_start;
        if (chunk_len > out_len - 1) {
            chunk_len = out_len - 1;
        }
        copy_fct(out, parsing_context.tx + chunk_start, chunk_len);
        out[chunk_len] = 0;
    }

    return num_chunks;
}

///// Update value characters from json transaction read from the token_index element.
///// Value is only updated if current_item_index (which is incremented internally) matches item_index_to_display
///// If value is updated, we also update view_scrolling_total_size to value string length.
int16_t retrieve_value(display_context_params_t *p, int16_t token_index) {
    if (p->current_item_index == p->item_index_to_display) {
        update(p->value, p->value_length, token_index, p->chunk_index);
        return p->item_index_to_display;
    }

    p->current_item_index++;
    return -1;
}

///// Update key characters from json transaction read from the token_index element.
void append_key_item(display_context_params_t *p, int16_t token_index) {
    if (*p->key > 0) {
        // There is already something there, add separator
        strcat_s(p->key, p->key_length, "/", 1);
    }

    const int16_t token_start = parsing_context.parsed_tx->Tokens[token_index].start;
    const int16_t token_end = parsing_context.parsed_tx->Tokens[token_index].end;
    const char *address_ptr = parsing_context.tx + token_start;
    const int16_t new_item_size = token_end - token_start;

    strcat_s(p->key, p->key_length, address_ptr, new_item_size);
}

int16_t display_arbitrary_item_inner(display_context_params_t *p, int16_t token_index, uint8_t level, uint8_t depth) {
//    if level == 2
//        show value as json-encoded string
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

    const jsmntype_t token_type = parsing_context.parsed_tx->Tokens[token_index].type;

    if (level >= MAX_TREE_LEVEL ||
        depth >= MAX_RECURSION_DEPTH ||
        token_type == JSMN_STRING ||
        token_type == JSMN_PRIMITIVE) {
        // Early bail out
        return retrieve_value(p, token_index);
    }

    const int16_t el_count = object_get_element_count(token_index, parsing_context.parsed_tx);
    const int16_t key_len = strlen(p->key);

    switch (token_type) {
        case JSMN_OBJECT: {
            for (int16_t i = 0; i < el_count; ++i) {
                int16_t key_index = object_get_nth_key(token_index, i, parsing_context.parsed_tx);
                int16_t value_index = object_get_nth_value(token_index, i, parsing_context.parsed_tx);

                *(p->key + key_len) = 0;
                if (p->item_index_to_display != -1) {
                    append_key_item(p, key_index);
                }

                int16_t found = display_arbitrary_item_inner(p, value_index, level + 1, depth + 1);
                if (p->item_index_to_display != -1 && p->item_index_to_display == found) {
                    return found;
                }
            }
            break;
        }
        case JSMN_ARRAY: {
            for (int16_t i = 0; i < el_count; ++i) {
                int16_t element_index = array_get_nth_element(token_index, i, parsing_context.parsed_tx);

                int16_t found = display_arbitrary_item_inner(p, element_index, level, depth + 1);
                if (p->item_index_to_display != -1 && p->item_index_to_display == found) {
                    return found;
                }
            }
            break;
        }
        default:
            return p->current_item_index;
    }

    // Not found yet, continue parsing
    return -1;
}

int16_t display_get_arbitrary_items_count(int16_t token_index) {

    int16_t number_of_items = 0;
    int16_t chunk_index = 0;
    char dummy[1];

    display_context_params_t params;
    params.item_index_to_display = -1;
    params.key = dummy;
    params.value = dummy;
    params.key_length = 1;
    params.value_length = 1;
    params.current_item_index = number_of_items;
    params.chunk_index = chunk_index;
    params.num_chunks = 0;

    const int8_t start_level = 0;
    const int8_t start_depth = 0;
    display_arbitrary_item_inner(&params, token_index, start_level, start_depth);

    return number_of_items;
}

int16_t display_arbitrary_item(int16_t item_index_to_display,
                               char *key, int16_t key_length,
                               char *value, int16_t value_length,
                               int16_t token_index, int16_t chunk_index) {

    // TODO: Simplify chunk index vs chunk max

    int16_t current_item_index = 0;

    display_context_params_t params;
    params.item_index_to_display = item_index_to_display;
    params.key = key;
    params.value = value;
    params.key_length = key_length;
    params.value_length = value_length;
    params.current_item_index = current_item_index;
    params.chunk_index = chunk_index;

    params.key[0] = 0;
    params.value[0] = 0;

    const int8_t start_level = 0;
    const int8_t start_depth = 0;

    return display_arbitrary_item_inner(&params, token_index, start_level, start_depth);
}

int16_t transaction_get_display_key_value(char *key, int16_t max_key_length,
                                          char *value, int16_t max_value_length,
                                          int16_t page_index,
                                          int16_t chunk_index) {
    const int16_t non_msg_pages_count = 5;
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
        int16_t token_index = object_get_value(ROOT_TOKEN_INDEX,
                                               key_name,
                                               parsing_context.parsed_tx,
                                               parsing_context.tx);

        update(value, max_value_length, token_index, chunk_index);
    } else {
        int16_t msgs_page_to_display = page_index - non_msg_pages_count;
        int16_t subpage_to_display = msgs_page_to_display;
        if (msgs_page_to_display < msgs_total_pages) {
            int16_t msgs_array_token_index = object_get_value(
                    ROOT_TOKEN_INDEX,
                    "msgs",
                    parsing_context.parsed_tx,
                    parsing_context.tx);

            int16_t total = 0;
            int16_t msgs_array_index = 0;
            int16_t msgs_token_index = 0;
            for (int16_t i = 0; i < msgs_array_elements; i++) {
                int16_t token_index_of_msg = array_get_nth_element(msgs_array_token_index, i,
                                                                   parsing_context.parsed_tx);
                int16_t count = display_get_arbitrary_items_count(token_index_of_msg);
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

int16_t transaction_get_display_pages() {
    int16_t msgs_token_index = object_get_value(
            ROOT_TOKEN_INDEX,
            "msgs",
            parsing_context.parsed_tx,
            parsing_context.tx);

    msgs_array_elements = array_get_element_count(msgs_token_index, parsing_context.parsed_tx);

    msgs_total_pages = 0;
    for (int16_t i = 0; i < msgs_array_elements; i++) {
        int16_t token_index_of_msg = array_get_nth_element(msgs_token_index, i, parsing_context.parsed_tx);
        msgs_total_pages += display_get_arbitrary_items_count(token_index_of_msg);
    }
    return msgs_total_pages + 5;
}

int8_t is_space(char c) {
    for (unsigned int i = 0; i < sizeof(whitespaces); i++) {
        if (whitespaces[i] == c) {
            return 1;
        }
    }
    return 0;
}

int8_t contains_whitespace(parsed_json_t *parsed_transaction,
                           const char *transaction) {

    int start = 0;
    int last_element_index = parsed_transaction->Tokens[0].end;

    // Starting at token 1 because token 0 contains full tx
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

int8_t is_sorted(int16_t first_index,
                 int16_t second_index,
                 parsed_json_t *parsed_transaction,
                 const char *transaction) {
#if DEBUG_SORTING
    char first[256];
    char second[256];

    int size =  parsed_tx->Tokens[first_index].end - parsed_tx->Tokens[first_index].start;
    strncpy(first, tx + parsed_tx->Tokens[first_index].start, size);
    first[size] = '\0';
    size =  parsed_tx->Tokens[second_index].end - parsed_tx->Tokens[second_index].start;
    strncpy(second, tx + parsed_tx->Tokens[second_index].start, size);
    second[size] = '\0';
#endif

    if (strcmp(
            transaction + parsed_transaction->Tokens[first_index].start,
            transaction + parsed_transaction->Tokens[second_index].start) <= 0) {
        return 1;
    }
    return 0;
}

int8_t dictionaries_sorted(parsed_json_t *parsed_transaction,
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