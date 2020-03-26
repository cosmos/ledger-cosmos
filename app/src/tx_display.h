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
#include <common/parser_common.h>
#include "parser_txdef.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    root_item_chain_id = 0,
    root_item_account_number = 1,
    root_item_sequence = 2,
    root_item_fee = 3,
    root_item_memo = 4,
    root_item_msgs = 5,
} root_item_e;

const char *get_required_root_item(root_item_e i);

parser_error_t tx_display_query(uint16_t displayIdx,
                                char *outKey,
                                uint16_t outKeyLen,
                                uint16_t *ret_value_token_index);

parser_error_t tx_display_readTx(parser_context_t *c,
                                 const uint8_t *data,
                                 size_t dataLen);

/// Return number of UI pages that we'll have for the current json transaction (only if the tx is valid)
/// \return number of pages (msg pages + 5 required)
uint16_t tx_display_numItems();

void tx_display_make_friendly();

//---------------------------------------------

#ifdef __cplusplus
}
#endif
