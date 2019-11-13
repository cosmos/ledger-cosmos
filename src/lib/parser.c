/*******************************************************************************
*   (c) 2019 ZondaX GmbH
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
#include <json/tx_validate.h>
#include <zxtypes.h>
#include "json/tx_parser.h"
#include "json/tx_display.h"
#include "lib/parser_impl.h"
#include "view_internal.h"
#include "jsmn.h"
#include "parser.h"

parser_error_t parser_parse(parser_context_t *ctx,
                            const uint8_t *data,
                            uint16_t dataLen) {
    parser_init(ctx, data, dataLen);
    return _readTx(ctx, &parser_tx_obj);
}

parser_error_t parser_validate(parser_context_t *ctx) {
    parser_error_t err = tx_validate(&parser_tx_obj.json);
    if (err != parser_ok)
        return err;

    // Iterate through all items to check that all can be shown and are valid

    uint8_t numItems = parser_getNumItems(ctx);

    char tmpKey[40];
    char tmpVal[40];

    for (uint8_t idx = 0; idx < numItems; idx++) {
        uint8_t pageCount;
        err = parser_getItem(ctx, idx, tmpKey, sizeof(tmpKey), tmpVal, sizeof(tmpVal), 0, &pageCount);
        if (err != parser_ok) {
            return err;
        }
    }

    return parser_ok;
}

uint8_t parser_getNumItems(parser_context_t *ctx) {
    return tx_display_numItems();
}

__Z_INLINE bool_t parser_areEqual(uint16_t tokenidx, char *expected) {
    if (parser_tx_obj.json.tokens[tokenidx].type != JSMN_STRING) {
        return false;
    }

    uint16_t len = parser_tx_obj.json.tokens[tokenidx].end - parser_tx_obj.json.tokens[tokenidx].start;
    if (strlen(expected) != len) {
        return false;
    }

    const char *p = parser_tx_obj.tx + parser_tx_obj.json.tokens[tokenidx].start;
    for (uint16_t i = 0; i < len; i++) {
        if (expected[i] != *(p + i)) {
            return false;
        }
    }

    return true;
}

__Z_INLINE bool_t parser_isAmount(char *key) {
    if (strcmp(parser_tx_obj.query.out_key, "fee/amount") == 0)
        return true;

    if (strcmp(parser_tx_obj.query.out_key, "msgs/inputs/coins") == 0)
        return true;

    if (strcmp(parser_tx_obj.query.out_key, "msgs/outputs/coins") == 0)
        return true;

    if (strcmp(parser_tx_obj.query.out_key, "msgs/value/amount") == 0)
        return true;

    return false;
}

__Z_INLINE parser_error_t parser_formatAmount(uint16_t amountToken,
                                              char *outVal, uint16_t outValLen,
                                              uint8_t pageIdx, uint8_t *pageCount) {
    uint16_t numElements = array_get_element_count(amountToken, &parser_tx_obj.json);
    if (parser_tx_obj.json.tokens[amountToken].type == JSMN_ARRAY) {
        amountToken++;
    }

    numElements = array_get_element_count(amountToken, &parser_tx_obj.json);
    if (numElements == 0) {
        snprintf(outVal, outValLen, "Empty");
        return parser_ok;
    }

    if (numElements != 4)
        return parser_unexpected_field;

    if (parser_tx_obj.json.tokens[amountToken].type != JSMN_OBJECT)
        return parser_unexpected_field;

    if (!parser_areEqual(amountToken + 1, "amount"))
        return parser_unexpected_field;

    if (!parser_areEqual(amountToken + 3, "denom"))
        return parser_unexpected_field;

    char bufferUI[160];
    MEMZERO(outVal, outValLen);
    MEMZERO(bufferUI, sizeof(bufferUI));

    const char *amountPtr = parser_tx_obj.tx + parser_tx_obj.json.tokens[amountToken + 2].start;
    const uint16_t amountLen = parser_tx_obj.json.tokens[amountToken + 2].end -
                               parser_tx_obj.json.tokens[amountToken + 2].start;
    const char *denomPtr = parser_tx_obj.tx + parser_tx_obj.json.tokens[amountToken + 4].start;
    const uint16_t denomLen = parser_tx_obj.json.tokens[amountToken + 4].end -
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

parser_error_t parser_getItem(parser_context_t *ctx,
                              int8_t displayIdx,
                              char *outKey, uint16_t outKeyLen,
                              char *outVal, uint16_t outValLen,
                              uint8_t pageIdx, uint8_t *pageCount) {

    MEMZERO(outKey, outKeyLen);
    MEMZERO(outVal, outValLen);
    INIT_QUERY(outKey, outKeyLen, outVal, outValLen, pageIdx)
    snprintf(outKey, outKeyLen, "?");
    snprintf(outVal, outValLen, " ");

    uint16_t displayStartToken;
    parser_error_t err = tx_display_set_query(displayIdx, &displayStartToken);
    if (err != parser_ok)
        return err;

    STRNCPY_S(parser_tx_obj.query.out_key,
              get_required_root_item(parser_tx_obj.query.item_index_root),
              parser_tx_obj.query.out_key_len)

    uint16_t ret_value_token_index;
    err = tx_traverse_find(displayStartToken, &ret_value_token_index);
    if (err != parser_ok)
        return err;

    if (parser_isAmount(parser_tx_obj.query.out_key)) {
        err = parser_formatAmount(ret_value_token_index, outVal, outValLen, pageIdx, pageCount);
    } else {
        err = tx_getToken(ret_value_token_index, outVal, outValLen, parser_tx_obj.query.chunk_index, pageCount);
    }

    tx_display_make_friendly();

    if (*pageCount > 1) {
        uint8_t keyLen = strlen(outKey);
        if (keyLen < outKeyLen) {
            snprintf(outKey + keyLen, outKeyLen - keyLen, " [%d/%d]", pageIdx + 1, *pageCount);
        }
    }

    return err;
}
