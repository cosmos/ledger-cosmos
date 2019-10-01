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

#pragma once

#include "json/json_parser.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TX_TOKEN_NOT_FOUND (-1)

#define MAX_RECURSION_DEPTH  6

#define INIT_QUERY_CONTEXT(_KEY, _KEY_LEN, _VAL, _VAL_LEN, _CHUNK_IDX, _MAX_LEVEL) \
    INIT_QUERY(_KEY, _KEY_LEN, _VAL, _VAL_LEN, _CHUNK_IDX) \
    parser_tx_obj.tx_ctx.item_index_current = 0; \
    parser_tx_obj.tx_ctx.max_depth = MAX_RECURSION_DEPTH; \
    parser_tx_obj.tx_ctx.max_level = _MAX_LEVEL;

#define INIT_QUERY(_KEY, _KEY_LEN, _VAL, _VAL_LEN, _CHUNK_IDX)  \
        _KEY[0] = 0; \
        _VAL[0] = 0; \
        parser_tx_obj.tx_ctx.query.out_key=_KEY; \
        parser_tx_obj.tx_ctx.query.out_val=_VAL; \
        parser_tx_obj.tx_ctx.query.out_key_len = _KEY_LEN; \
        parser_tx_obj.tx_ctx.query.out_val_len = _VAL_LEN; \
        parser_tx_obj.tx_ctx.query.item_index= 0; \
        parser_tx_obj.tx_ctx.query.chunk_index = _CHUNK_IDX;

// Traverses transaction data and fills tx_context
// \return -1 if the item was not found or the number of available chunks for this item
int16_t tx_traverse(int16_t root_token_index);

// Retrieves the value for the corresponding token index. If the value goes beyond val_len, the chunk_idx will be used
int16_t tx_get_value(int16_t token_index);

//---------------------------------------------

#ifdef __cplusplus
}
#endif
