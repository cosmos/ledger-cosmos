/*******************************************************************************
*   (c) 2018, 2019 ZondaX GmbH
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
#include <parser_common.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_RECURSION_DEPTH  6

#define INIT_QUERY_CONTEXT(_KEY, _KEY_LEN, _VAL, _VAL_LEN, _CHUNK_IDX, _MAX_LEVEL) \
    INIT_QUERY(_KEY, _KEY_LEN, _VAL, _VAL_LEN, _CHUNK_IDX) \
    parser_tx_obj.query.item_index_current = 0; \
    parser_tx_obj.query.max_depth = MAX_RECURSION_DEPTH; \
    parser_tx_obj.query.max_level = _MAX_LEVEL;

#define INIT_QUERY(_KEY, _KEY_LEN, _VAL, _VAL_LEN, _CHUNK_IDX)  \
        _KEY[0] = 0; \
        _VAL[0] = 0; \
        parser_tx_obj.query.out_key=_KEY; \
        parser_tx_obj.query.out_val=_VAL; \
        parser_tx_obj.query.out_key_len = _KEY_LEN; \
        parser_tx_obj.query.out_val_len = _VAL_LEN; \
        parser_tx_obj.query.item_index= 0; \
        parser_tx_obj.query.chunk_index = _CHUNK_IDX;

parser_error_t tx_traverse_find(int16_t root_token_index, uint16_t *ret_value_token_index);

// Traverses transaction data and fills tx_context
parser_error_t tx_traverse(int16_t root_token_index, uint8_t *numChunks);

// Retrieves the value for the corresponding token index. If the value goes beyond val_len, the chunk_idx will be used

parser_error_t tx_getToken(uint16_t token_index,
                           char *out_val, uint16_t out_val_len,
                           uint8_t pageIdx, uint8_t *pageCount);
//---------------------------------------------

#ifdef __cplusplus
}
#endif
