/*******************************************************************************
 *   (c) 2016 Ledger
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

#include "lib_standard_app/swap_lib_calls.h"
#include "parser_common.h"
#include "stdbool.h"
#include "stdint.h"
#include "zxerror.h"
#include "parser_txdef.h"
#include "parser.h"

#define ADDRESS_MAXSIZE 50
#define MEMO_MAXSIZE 50
#define EXPERT_MODE_ITEMS 10
#define NORMAL_MODE_ITEMS 6
#define EXPERT_SEND_MODE_ITEMS 10
#define NORMAL_SEND_MODE_ITEMS 6

typedef struct {
    uint8_t amount[COIN_AMOUNT_MAXSIZE];
    uint8_t amount_length;
    uint8_t fees[COIN_AMOUNT_MAXSIZE];
    uint8_t fees_length;
    char destination_address[ADDRESS_MAXSIZE];
    /* Is swap mode */
    uint8_t called_from_swap;
    uint8_t should_exit;
    char memo[MEMO_MAXSIZE];
} swap_globals_t;

extern swap_globals_t G_swap_state;

// Handler for swap features
parser_error_t check_swap_conditions(parser_context_t *ctx_parsed_tx);
void handle_check_address(check_address_parameters_t *params);
void handle_get_printable_amount(get_printable_amount_parameters_t *params);
bool copy_transaction_parameters(create_transaction_parameters_t *sign_transaction_params);
void __attribute__((noreturn)) finalize_exchange_sign_transaction(bool is_success);
