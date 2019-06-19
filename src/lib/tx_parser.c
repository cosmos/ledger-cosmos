/*******************************************************************************
*   (c) ZondaX GmbH
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
#include "tx_parser.h"
#include "json_parser.h"

#if defined(TARGET_NANOX) || defined(TARGET_NANOS)
#include "os.h"
#define COPYFUNC os_memmove
#else
#define COPYFUNC memcpy
#define __always_inline
#endif

// Global context to save memory / stack space in recursive calls
parsing_context_t parsing_context;
tx_context_t tx_ctx;

void set_parsing_context(parsing_context_t context) {
    parsing_context = context;
    // reset cached values
    parsing_context.cache_valid = false;
}

// strcat but source does not need to be terminated (a chunk from a bigger string is concatenated)
// dst_max is measured in bytes including the space for NULL termination
// src_size does not include NULL termination
__always_inline void strcat_chunk_s(char *dst, uint16_t dst_max, const char *src_chunk, uint16_t src_chunk_size) {
    *(dst + dst_max - 1) = 0;                 // last character terminates with zero in case we go beyond bounds
    const uint16_t prev_size = strlen(dst);

    uint16_t space_left = dst_max - prev_size - 1;  // -1 because requires termination

    if (src_chunk_size > space_left) {
        src_chunk_size = space_left;
    }

    if (src_chunk_size > 0) {
        // Check bounds
        COPYFUNC(dst + prev_size, src_chunk, src_chunk_size);
        // terminate
        *(dst + prev_size + src_chunk_size) = 0;
    }
}

__always_inline int16_t tx_get_value(const int16_t token_index) {

    const int16_t token_start = parsing_context.parsed_tx->Tokens[token_index].start;
    const int16_t token_end = parsing_context.parsed_tx->Tokens[token_index].end;
    const int16_t token_len = token_end - token_start;

    int16_t num_chunks = (token_len / (tx_ctx.query.out_val_len - 1)) + 1;
    if (token_len > 0 && (token_len % (tx_ctx.query.out_val_len - 1) == 0))
        num_chunks--;

    tx_ctx.query.out_val[0] = '\0';  // flush
    if (tx_ctx.query.chunk_index >= num_chunks) {
        return TX_TOKEN_NOT_FOUND;
    }

    const int16_t chunk_start = token_start + tx_ctx.query.chunk_index * (tx_ctx.query.out_val_len - 1);
    int16_t chunk_len = token_end - chunk_start;

    if (chunk_len < 0) {
        return TX_TOKEN_NOT_FOUND;
    }

    if (chunk_len > tx_ctx.query.out_val_len - 1) {
        chunk_len = tx_ctx.query.out_val_len - 1;
    }
    COPYFUNC(tx_ctx.query.out_val, parsing_context.tx + chunk_start, chunk_len);
    tx_ctx.query.out_val[chunk_len] = 0;

    return num_chunks;
}

///// Update key characters from json transaction read from the token_index element.
__always_inline void append_key_item(int16_t token_index) {
    if (*tx_ctx.query.out_key > 0) {
        // There is already something there, add separator
        strcat_chunk_s(tx_ctx.query.out_key, tx_ctx.query.out_key_len, "/", 1);
    }

    const int16_t token_start = parsing_context.parsed_tx->Tokens[token_index].start;
    const int16_t token_end = parsing_context.parsed_tx->Tokens[token_index].end;
    const char *address_ptr = parsing_context.tx + token_start;
    const int16_t new_item_size = token_end - token_start;

    strcat_chunk_s(tx_ctx.query.out_key, tx_ctx.query.out_key_len, address_ptr, new_item_size);
}

int16_t tx_traverse(int16_t root_token_index) {
    const jsmntype_t token_type = parsing_context.parsed_tx->Tokens[root_token_index].type;

    if (tx_ctx.max_level <= 0 || tx_ctx.max_depth <= 0 ||
        token_type == JSMN_STRING ||
        token_type == JSMN_PRIMITIVE) {

        // Early bail out
        if (tx_ctx.item_index_current == tx_ctx.query.item_index) {
            return tx_get_value(root_token_index);
        }
        tx_ctx.item_index_current++;
        return TX_TOKEN_NOT_FOUND;
    }

    const int16_t el_count = object_get_element_count(root_token_index, parsing_context.parsed_tx);
    int16_t num_chunks = TX_TOKEN_NOT_FOUND;

    switch (token_type) {
        case JSMN_OBJECT: {
            const int16_t key_len = strlen(tx_ctx.query.out_key);
            for (int16_t i = 0; i < el_count; ++i) {
                const int16_t key_index = object_get_nth_key(root_token_index, i, parsing_context.parsed_tx);
                const int16_t value_index = object_get_nth_value(root_token_index, i, parsing_context.parsed_tx);

                // Skip writing keys if we are actually exploring to count
                if (tx_ctx.query.item_index != TX_TOKEN_NOT_FOUND) {
                    append_key_item(key_index);
                }

                // When traversing objects both level and depth should be considered
                tx_ctx.max_level--;
                tx_ctx.max_depth--;
                num_chunks = tx_traverse(value_index);       // Traverse the value, extracting subkeys
                tx_ctx.max_level++;
                tx_ctx.max_depth++;

                if (num_chunks != TX_TOKEN_NOT_FOUND) {
                    break;
                }

                *(tx_ctx.query.out_key + key_len) = 0;
            }
            break;
        }
        case JSMN_ARRAY: {
            for (int16_t i = 0; i < el_count; ++i) {
                const int16_t element_index = array_get_nth_element(root_token_index, i, parsing_context.parsed_tx);

                // When iterating along an array, the level does not change but we need to count the recursion
                tx_ctx.max_depth--;
                num_chunks = tx_traverse(element_index);
                tx_ctx.max_depth++;

                if (num_chunks != TX_TOKEN_NOT_FOUND) {
                    break;
                }
            }
            break;
        }
        default:
            break;
    }

    return num_chunks;
}
