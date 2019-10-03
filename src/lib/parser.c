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
#include <json/tx_validate.h>
#include <zxmacros.h>
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
    return tx_validate(&parser_tx_obj.json);
}

uint8_t parser_getNumItems(parser_context_t *ctx) {
    return tx_display_numItems();
}

parser_error_t parser_getItem(parser_context_t *ctx,
                              int8_t displayIdx,
                              char *outKey, uint16_t outKeyLen,
                              char *outVal, uint16_t outValLen,
                              uint8_t pageIdx, uint8_t *pageCount) {

    MEMSET(outKey, 0, outKeyLen);
    MEMSET(outVal, 0, outValLen);
    snprintf(outKey, outKeyLen, "?");
    snprintf(outVal, outValLen, "?");

    if (displayIdx < 0) {
        return parser_no_data;
    }

    INIT_QUERY(outKey, outKeyLen, outVal, outValLen, pageIdx)

    uint16_t displayStartToken;
    parser_error_t err = tx_display_set_query(displayIdx, &displayStartToken);
    if (err != parser_ok)
        return err;

    STRNCPY_S(parser_tx_obj.query.out_key,
              get_required_root_item(parser_tx_obj.item_index_root),
              parser_tx_obj.query.out_key_len)

    uint16_t ret_value_token_index;
    err = tx_traverse_find(displayStartToken, &ret_value_token_index);

    if (err != parser_ok)
        return err;

    err = tx_getToken(ret_value_token_index,
                      parser_tx_obj.query.out_val, parser_tx_obj.query.out_val_len,
                      parser_tx_obj.query.chunk_index, pageCount);
    if (err != parser_ok)
        return err;

    tx_display_make_friendly();

    return err;
}
