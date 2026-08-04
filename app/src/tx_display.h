/*******************************************************************************
 *   (c) 2018, 2019 Zondax GmbH
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

#include "coin.h"
#include "parser_impl.h"
#include "parser_txdef.h"
#include <common/parser_common.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// The review UI reaches items through zxlib's viewfunc_getItem_t, whose
// displayIdx is int8_t. An index above 127 arrives negative and is refused, so
// items beyond this point cannot be shown even though the parser can render
// them. Count no further than the screen can reach.
//
// 127 is exact, not a round number. The Nano review calls getItem once more at
// index == numItems as its end-of-list sentinel, so the largest index the
// callback ever sees is 127, which is INT8_MAX. Raising this to 128 makes that
// sentinel arrive as -128 and puts the narrowing straight back.
#define MAX_REVIEW_ITEMS 127

typedef enum {
  root_item_chain_id = 0,
  root_item_account_number,
  root_item_sequence,
  root_item_msgs,
  root_item_memo,
  root_item_fee,
  root_item_tip,
  root_item_timeout_height,
} root_item_e;

parser_error_t
tx_is_expert_mode_or_not_default_chainid(bool *expert_or_default);

const char *get_required_root_item(root_item_e i);

parser_error_t tx_display_query(uint16_t displayIdx, char *outKey,
                                uint16_t outKeyLen,
                                uint16_t *ret_value_token_index);

parser_error_t tx_display_numItems(uint8_t *num_items);

parser_error_t tx_display_make_friendly();

parser_error_t tx_display_translation(char *dst, uint16_t dstLen, char *src,
                                      uint16_t srcLen);
//---------------------------------------------

#ifdef __cplusplus
}
#endif
