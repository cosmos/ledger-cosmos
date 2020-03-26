/*******************************************************************************
*   (c) 2019 Zondax GmbH
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

#include <stdio.h>
#include <zxmacros.h>
#include <tx_validate.h>
#include <zxtypes.h>
#include "tx_parser.h"
#include "tx_display.h"
#include "parser_impl.h"
#include "common/parser.h"

__Z_INLINE parser_error_t parser_getItem_raw(const parser_context_t *ctx,
                                             int8_t displayIdx,
                                             char *outKey, uint16_t outKeyLen,
                                             char *outVal, uint16_t outValLen,
                                             uint8_t pageIdx, uint8_t *pageCount);

parser_error_t parser_parse(parser_context_t *ctx,
                            const uint8_t *data,
                            size_t dataLen) {
    CHECK_PARSER_ERR(tx_display_readTx(ctx, data, dataLen))
    return parser_ok;
}

parser_error_t parser_validate(const parser_context_t *ctx) {
    CHECK_PARSER_ERR(tx_validate(&parser_tx_obj.json))

    // Iterate through all items to check that all can be shown and are valid
    uint8_t numItems = parser_getNumItems(ctx);

    char tmpKey[40];
    char tmpVal[40];

    for (uint8_t idx = 0; idx < numItems; idx++) {
        uint8_t pageCount;
        CHECK_PARSER_ERR(parser_getItem(ctx, idx, tmpKey, sizeof(tmpKey), tmpVal, sizeof(tmpVal), 0, &pageCount))
    }

    return parser_ok;
}

uint8_t parser_getNumItems(const parser_context_t *ctx) {
    return tx_display_numItems();
}

__Z_INLINE bool_t parser_areEqual(uint16_t tokenidx, char *expected) {
    if (parser_tx_obj.json.tokens[tokenidx].type != JSMN_STRING) {
        return bool_false;
    }

    int16_t len = parser_tx_obj.json.tokens[tokenidx].end - parser_tx_obj.json.tokens[tokenidx].start;
    if (strlen(expected) != len) {
        return bool_false;
    }

    const char *p = parser_tx_obj.tx + parser_tx_obj.json.tokens[tokenidx].start;
    for (uint16_t i = 0; i < len; i++) {
        if (expected[i] != *(p + i)) {
            return bool_false;
        }
    }

    return bool_true;
}

__Z_INLINE bool_t parser_isAmount(char *key) {
    if (strcmp(parser_tx_obj.query.out_key, "fee/amount") == 0)
        return bool_true;

    if (strcmp(parser_tx_obj.query.out_key, "msgs/inputs/coins") == 0)
        return bool_true;

    if (strcmp(parser_tx_obj.query.out_key, "msgs/outputs/coins") == 0)
        return bool_true;

    if (strcmp(parser_tx_obj.query.out_key, "msgs/value/amount") == 0)
        return bool_true;

    return bool_false;
}

__Z_INLINE parser_error_t parser_formatAmount(uint16_t amountToken,
                                              char *outVal, uint16_t outValLen,
                                              uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 0;
    if (parser_tx_obj.json.tokens[amountToken].type == JSMN_ARRAY) {
        amountToken++;
    }

    uint16_t numElements = array_get_element_count(amountToken, &parser_tx_obj.json);
    if (numElements == 0) {
        *pageCount = 1;
        snprintf(outVal, outValLen, "Empty");
        return parser_ok;
    }

    if (numElements != 4)
        return parser_unexpected_field;

    if (parser_tx_obj.json.tokens[amountToken].type != JSMN_OBJECT)
        return parser_unexpected_field;

    if (!parser_areEqual(amountToken + 1u, "amount"))
        return parser_unexpected_field;

    if (!parser_areEqual(amountToken + 3u, "denom"))
        return parser_unexpected_field;

    char bufferUI[160];
    MEMZERO(outVal, outValLen);
    MEMZERO(bufferUI, sizeof(bufferUI));

    const char *amountPtr = parser_tx_obj.tx + parser_tx_obj.json.tokens[amountToken + 2].start;
    const int16_t amountLen = parser_tx_obj.json.tokens[amountToken + 2].end -
                              parser_tx_obj.json.tokens[amountToken + 2].start;
    const char *denomPtr = parser_tx_obj.tx + parser_tx_obj.json.tokens[amountToken + 4].start;
    const int16_t denomLen = parser_tx_obj.json.tokens[amountToken + 4].end -
                             parser_tx_obj.json.tokens[amountToken + 4].start;

    if (sizeof(bufferUI) < amountLen + denomLen + 2) {
        return parser_unexpected_buffer_end;
    }

    if (amountLen == 0) {
        return parser_unexpected_buffer_end;
    }

    if (denomLen == 0) {
        return parser_unexpected_buffer_end;
    }

    MEMCPY(bufferUI, amountPtr, amountLen);
    bufferUI[amountLen] = ' ';
    MEMCPY(bufferUI + 1 + amountLen, denomPtr, denomLen);

    pageString(outVal, outValLen, bufferUI, pageIdx, pageCount);

    return parser_ok;
}

parser_error_t parser_getItem(const parser_context_t *ctx,
                              uint16_t displayIdx,
                              char *outKey, uint16_t outKeyLen,
                              char *outVal, uint16_t outValLen,
                              uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 0;

    if (parser_getNumItems(ctx) == 0) {
        return parser_unexpected_number_items;
    }

    if (displayIdx < 0 || displayIdx >= parser_getNumItems(ctx)) {
        return parser_display_idx_out_of_range;
    }

    uint16_t ret_value_token_index;
    tx_display_query(displayIdx, outKey, outKeyLen, &ret_value_token_index);

    if (parser_isAmount(outKey)) {
        CHECK_PARSER_ERR(parser_formatAmount(
                ret_value_token_index,
                outVal, outValLen,
                pageIdx, pageCount))
    } else {
        CHECK_PARSER_ERR(tx_getToken(
                ret_value_token_index,
                outVal, outValLen,
                pageIdx, pageCount))
    }

    tx_display_make_friendly();

    if (*pageCount > 1) {
        size_t keyLen = strlen(outKey);
        if (keyLen < outKeyLen) {
            snprintf(outKey + keyLen, outKeyLen - keyLen, " [%d/%d]", pageIdx + 1, *pageCount);
        }
    }

    return parser_ok;
}
