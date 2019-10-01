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
#include "json/tx_parser.h"
#include "json/tx_display.h"
#include "lib/parser_impl.h"
#include "view_internal.h"

parser_error_t parser_parse(parser_context_t *ctx,
                            const uint8_t *data,
                            uint16_t dataLen) {
    parser_init(ctx, data, dataLen);
    return _readTx(ctx, &parser_tx_obj);
}

parser_error_t parser_validate(parser_context_t *ctx) {
    lastErrorMessage = tx_validate(&parser_tx_obj.json, (const char *) ctx->buffer);
    if (lastErrorMessage != NULL) {
        return parser_extended_error;
    }
    return parser_ok;
}

uint8_t parser_getNumItems(parser_context_t *ctx) {
    return tx_display_numItems();
}

parser_error_t parser_getItem(parser_context_t *ctx,
                              int8_t displayIdx,
                              char *outKey, uint16_t outKeyLen,
                              char *outValue, uint16_t outValueLen,
                              uint8_t pageIdx, uint8_t *pageCount) {

    snprintf(outKey, outKeyLen, "?");
    snprintf(outValue, outValueLen, "?");

    parser_error_t err = parser_ok;

    if (displayIdx < 0) {
        return parser_no_data;
    }

    INIT_QUERY(outKey, outKeyLen, outValue, outValueLen, pageIdx)
    int16_t ret = tx_display_get_item(displayIdx);

    if (ret == TX_TOKEN_NOT_FOUND) {
        return parser_no_data;
    }

    if (ret < 0 || ret > 255) {
        return parser_unexpected_buffer_end;
    }

    *pageCount = ret;
    tx_display_make_friendly();

    return err;
}
