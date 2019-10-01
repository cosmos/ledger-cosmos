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
#include "json/json_parser.h"
#include "zxmacros.h"
#include "parser_impl.h"

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
        MEMCPY(dst + prev_size, src_chunk, src_chunk_size);
        // terminate
        *(dst + prev_size + src_chunk_size) = 0;
    }
}

__always_inline int16_t tx_get_value(const int16_t token_index) {

    const int16_t token_start = parser_tx_obj.json.Tokens[token_index].start;
    const int16_t token_end = parser_tx_obj.json.Tokens[token_index].end;
    const int16_t token_len = token_end - token_start;

    int16_t num_chunks = (token_len / (parser_tx_obj.tx_ctx.query.out_val_len - 1)) + 1;
    if (token_len > 0 && (token_len % (parser_tx_obj.tx_ctx.query.out_val_len - 1) == 0))
        num_chunks--;

    parser_tx_obj.tx_ctx.query.out_val[0] = '\0';  // flush
    if (parser_tx_obj.tx_ctx.query.chunk_index >= num_chunks) {
        return TX_TOKEN_NOT_FOUND;
    }

    const int16_t chunk_start =
        token_start + parser_tx_obj.tx_ctx.query.chunk_index * (parser_tx_obj.tx_ctx.query.out_val_len - 1);
    int16_t chunk_len = token_end - chunk_start;

    if (chunk_len < 0) {
        return TX_TOKEN_NOT_FOUND;
    }

    if (chunk_len > parser_tx_obj.tx_ctx.query.out_val_len - 1) {
        chunk_len = parser_tx_obj.tx_ctx.query.out_val_len - 1;
    }
    MEMCPY(parser_tx_obj.tx_ctx.query.out_val, parser_tx_obj.tx + chunk_start, chunk_len);
    parser_tx_obj.tx_ctx.query.out_val[chunk_len] = 0;

    return num_chunks;
}

///// Update key characters from json transaction read from the token_index element.
__always_inline void append_key_item(int16_t token_index) {
    if (*parser_tx_obj.tx_ctx.query.out_key > 0) {
        // There is already something there, add separator
        strcat_chunk_s(parser_tx_obj.tx_ctx.query.out_key,
                       parser_tx_obj.tx_ctx.query.out_key_len, "/", 1);
    }

    const int16_t token_start = parser_tx_obj.json.Tokens[token_index].start;
    const int16_t token_end = parser_tx_obj.json.Tokens[token_index].end;
    const char *address_ptr = parser_tx_obj.tx + token_start;
    const int16_t new_item_size = token_end - token_start;

    strcat_chunk_s(parser_tx_obj.tx_ctx.query.out_key,
                   parser_tx_obj.tx_ctx.query.out_key_len, address_ptr, new_item_size);
}

int16_t tx_traverse(int16_t root_token_index) {
    const jsmntype_t token_type = parser_tx_obj.json.Tokens[root_token_index].type;

    if (parser_tx_obj.tx_ctx.max_level <= 0 || parser_tx_obj.tx_ctx.max_depth <= 0 ||
        token_type == JSMN_STRING ||
        token_type == JSMN_PRIMITIVE) {

        // Early bail out
        if (parser_tx_obj.tx_ctx.item_index_current == parser_tx_obj.tx_ctx.query.item_index) {
            return tx_get_value(root_token_index);
        }
        parser_tx_obj.tx_ctx.item_index_current++;
        return TX_TOKEN_NOT_FOUND;
    }

    const int16_t el_count = object_get_element_count(root_token_index, &parser_tx_obj.json);
    int16_t num_chunks = TX_TOKEN_NOT_FOUND;

    switch (token_type) {
        case JSMN_OBJECT: {
            const int16_t key_len = strlen(parser_tx_obj.tx_ctx.query.out_key);
            for (int16_t i = 0; i < el_count; ++i) {
                const int16_t key_index = object_get_nth_key(root_token_index, i, &parser_tx_obj.json);
                const int16_t value_index = object_get_nth_value(root_token_index, i, &parser_tx_obj.json);

                // Skip writing keys if we are actually exploring to count
                if (parser_tx_obj.tx_ctx.query.item_index != TX_TOKEN_NOT_FOUND) {
                    append_key_item(key_index);
                }

                // When traversing objects both level and depth should be considered
                parser_tx_obj.tx_ctx.max_level--;
                parser_tx_obj.tx_ctx.max_depth--;
                num_chunks = tx_traverse(value_index);       // Traverse the value, extracting subkeys
                parser_tx_obj.tx_ctx.max_level++;
                parser_tx_obj.tx_ctx.max_depth++;

                if (num_chunks != TX_TOKEN_NOT_FOUND) {
                    break;
                }

                *(parser_tx_obj.tx_ctx.query.out_key + key_len) = 0;
            }
            break;
        }
        case JSMN_ARRAY: {
            for (int16_t i = 0; i < el_count; ++i) {
                const int16_t element_index = array_get_nth_element(root_token_index, i, &parser_tx_obj.json);

                // When iterating along an array, the level does not change but we need to count the recursion
                parser_tx_obj.tx_ctx.max_depth--;
                num_chunks = tx_traverse(element_index);
                parser_tx_obj.tx_ctx.max_depth++;

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
