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

#include <stdint.h>
#include "parser_impl.h"
#include <common/parser_common.h>
#include "parser_txdef.h"
#include "coin.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    root_item_chain_id = 0,
    root_item_account_number,
    root_item_sequence,
    root_item_msgs,
    root_item_memo,
    root_item_fee,
    root_item_tip,
} root_item_e;

bool tx_is_expert_mode();

const char *get_required_root_item(root_item_e i);

parser_error_t tx_display_query(uint16_t displayIdx,
                                char *outKey, uint16_t outKeyLen,
                                uint16_t *ret_value_token_index);

parser_error_t tx_display_numItems(uint8_t *num_items);

parser_error_t tx_display_make_friendly();

parser_error_t tx_display_translation(char *dst, uint16_t dstLen, char *src, uint16_t srcLen);
//---------------------------------------------

#ifdef __cplusplus
}
#endif
