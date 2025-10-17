/*******************************************************************************
 *   (c) 2018 - 2023 Zondax AG
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

#ifdef __cplusplus
#pragma clang diagnostic push
#pragma ide diagnostic ignored "modernize-deprecated-headers"
#endif

#include "zxmacros.h"
#include "json/json_parser.h"
#include <common/parser_common.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_RECURSION_DEPTH 6
extern bool extraDepthLevel;

#define INIT_QUERY_CONTEXT(_KEY, _KEY_LEN, _VAL, _VAL_LEN, _PAGE_IDX,          \
                           _MAX_LEVEL)                                         \
  parser_tx_obj.tx_json.query._item_index_current = 0;                         \
  parser_tx_obj.tx_json.query.max_depth = MAX_RECURSION_DEPTH;                 \
  parser_tx_obj.tx_json.query.max_level = _MAX_LEVEL;                          \
                                                                               \
  parser_tx_obj.tx_json.query.item_index = 0;                                  \
  parser_tx_obj.tx_json.query.page_index = (_PAGE_IDX);                        \
                                                                               \
  MEMZERO(_KEY, (_KEY_LEN));                                                   \
  MEMZERO(_VAL, (_VAL_LEN));                                                   \
  parser_tx_obj.tx_json.query.out_key = _KEY;                                  \
  parser_tx_obj.tx_json.query.out_val = _VAL;                                  \
  parser_tx_obj.tx_json.query.out_key_len = (_KEY_LEN);                        \
  parser_tx_obj.tx_json.query.out_val_len = (_VAL_LEN);

parser_error_t tx_traverse_find(uint16_t root_token_index,
                                uint16_t *ret_value_token_index);

// Traverses transaction data and fills tx_context
parser_error_t tx_traverse(int16_t root_token_index, uint8_t *numChunks);

// Retrieves the value for the corresponding token index. If the value goes
// beyond val_len, the chunk_idx will be used
parser_error_t tx_getToken(uint16_t token_index, char *out_val,
                           uint16_t out_val_len, uint8_t pageIdx,
                           uint8_t *pageCount);

__Z_INLINE bool is_msg_type_field(char *field_name) {
  return strcmp(field_name, "msgs/type") == 0;
}

__Z_INLINE bool is_msg_from_field(char *field_name) {
  return strcmp(field_name, "msgs/value/delegator_address") == 0;
}

#ifdef __cplusplus
}
#pragma clang diagnostic pop
#endif
