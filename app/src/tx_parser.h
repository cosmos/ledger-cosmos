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
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_RECURSION_DEPTH 6

// Flatten depth (query.max_level) used when enumerating messages inside the
// `msgs` array. A MsgMultiSend nests its coins one level deeper
// (value/inputs|outputs/coins) and needs MSG_MULTISEND_FLATTEN_LEVEL; every
// other message — including the legacy shape that carries no "type" field —
// renders correctly at MSG_BASE_FLATTEN_LEVEL. The depth is applied per message
// in tx_traverse_find, derived from each message's own type; get_root_max_level
// uses the larger value only as a safe ceiling for entering the array.
#define MSG_BASE_FLATTEN_LEVEL 2
#define MSG_MULTISEND_FLATTEN_LEVEL 3
// Babylon x/epoching wrapped staking messages (epoching/Wrapped*) nest a
// standard staking message under a "msg" key, one object level deeper than the
// base shape, so they need the same extra flatten level as MsgMultiSend.
#define MSG_EPOCHING_FLATTEN_LEVEL 3
// cosmos-sdk/MsgExec carries whole messages of its own under value/msgs, two
// object levels below where a plain message keeps its fields. Reaching them
// needs both a deeper flatten level and a deeper recursion budget: at the base
// budget the walk runs out at the inner "value" and renders the message the
// grantee is about to execute as one JSON blob.
#define MSG_EXEC_FLATTEN_LEVEL 4
#define MSG_EXEC_RECURSION_DEPTH 8

// Returns the flatten depth a single `msgs` element needs, from its own type.
uint8_t tx_msg_max_level(uint16_t msg_token_index);

// Returns the recursion budget a single `msgs` element needs, from its own
// type. Only the shapes that nest messages inside messages need more than
// MAX_RECURSION_DEPTH, and only for their own subtree.
uint8_t tx_msg_max_depth(uint16_t msg_token_index);

// Chooses whether the messages a MsgExec carries get screens of their own, and
// resets the observation tx_msg_saw_exec reports. Itemising them is what makes
// a grantee batch legible, but each executed message costs several review
// items and the review index stops at MAX_REVIEW_ITEMS; a batch too large to
// itemise is rendered at MSG_BASE_FLATTEN_LEVEL instead -- the executed
// messages as one paged JSON value, which is what the app showed before they
// were given screens -- so a document that could be reviewed and signed before
// still can be. tx_display owns the choice: it is taken once per document while
// indexing, so counting the items and rendering them enumerate the same set.
void tx_msg_itemize_exec(bool enabled);

// True if the walk since the last tx_msg_itemize_exec call met a MsgExec, so
// the choice is only reconsidered for documents it can change.
bool tx_msg_saw_exec(void);

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
  if (field_name == NULL) {
    return false;
  }
  return strcmp(field_name, "msgs/type") == 0;
}

__Z_INLINE bool is_msg_from_field(char *field_name) {
  if (field_name == NULL) {
    return false;
  }
  // The base staking shape carries the delegator at
  // msgs/value/delegator_address; Babylon epoching/Wrapped* messages carry it
  // one level deeper, under the nested "msg" object.
  return strcmp(field_name, "msgs/value/delegator_address") == 0 ||
         strcmp(field_name, "msgs/value/msg/delegator_address") == 0;
}

#ifdef __cplusplus
}
#pragma clang diagnostic pop
#endif
