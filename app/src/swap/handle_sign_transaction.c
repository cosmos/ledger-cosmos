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
#include "bignum.h"
#include "crypto.h"
#include "lib_standard_app/swap_lib_calls.h"
#include "parser.h"
#include "swap.h"
#include "swap_utils.h"
#include "zxformat.h"
#include "app_mode.h"

swap_globals_t G_swap_state;

// Save the BSS address where we will write the return value when finished
static uint8_t *G_swap_sign_return_value_address;

bool copy_transaction_parameters(create_transaction_parameters_t *sign_transaction_params) {
    if (sign_transaction_params == NULL) {
        return false;
    }

    // First copy parameters to stack, and then to global data.
    // We need this "trick" as the input data position can overlap with globals
    char destination_address[ADDRESS_MAXSIZE] = {0};
    uint8_t amount[COIN_AMOUNT_MAXSIZE] = {0};
    uint8_t amount_length = {0};
    uint8_t fees[COIN_AMOUNT_MAXSIZE] = {0};
    uint8_t fees_length = {0};
    char memo[MEMO_MAXSIZE] = {0};

    // Check and copy destination address
    if ((destination_address[sizeof(destination_address) - 1] != '\0') || (sign_transaction_params->amount_length > COIN_AMOUNT_MAXSIZE) ||
        (sign_transaction_params->fee_amount_length > COIN_AMOUNT_MAXSIZE)) {
        return false;
    }
    strncpy(destination_address, sign_transaction_params->destination_address, sizeof(destination_address));

    // Check and copy memo
    if(strlen(sign_transaction_params->destination_address_extra_id) >= sizeof(G_swap_state.memo)) {
        return false;
    }
    strncpy(memo, sign_transaction_params->destination_address_extra_id, sizeof(G_swap_state.memo));

    // Check and copy amount
    memcpy(amount, sign_transaction_params->amount, sign_transaction_params->amount_length);
    amount_length = sign_transaction_params->amount_length;

    // Check and copy fees
    memcpy(fees, sign_transaction_params->fee_amount, sign_transaction_params->fee_amount_length);
    fees_length = sign_transaction_params->fee_amount_length;

    // Full reset the global variables
    os_explicit_zero_BSS_segment();

    // Keep the address at which we'll reply the signing status
    G_swap_sign_return_value_address = &sign_transaction_params->result;

    // Commit the values read from exchange to the clean global space
    G_swap_state.amount_length = amount_length;
    memcpy(G_swap_state.amount, amount, sizeof(amount));
    G_swap_state.fees_length = fees_length;
    memcpy(G_swap_state.fees, fees, sizeof(G_swap_state.fees));
    memcpy(G_swap_state.destination_address, destination_address, sizeof(G_swap_state.destination_address));
    memcpy(G_swap_state.memo, memo, sizeof(G_swap_state.memo));

    return true;
}

/*
 * This function verifies that a received transaction follows the expected format
 * based on the current application mode (expert or normal). The verification
 * process includes checking the number of items in the transaction and validating
 * that the items at its respective display index matches the expected content.
 * If any item does not meet the expected criteria, the function will return an error.
 *
 * Expected transaction format:
 *
 * Expert Mode:
 * 0 | Chain ID : cosmoshub-4
 * 1 | Account : 3225600
 * 2 | Sequence : 0
 * 3 | Type : Send
 * 4 | Amount : 6000000 ATOM
 * 5 | From [1/2] : cosmos12fuhfs6juf3m47dgplrhdrkvdyn9wc2j
 * 5 | From [2/2] : mp4sw40
 * 6 | To [1/2] : cosmos1s3mu2eyc7ql5hrcqvuerfhxxfxs7ax4k
 * 7 | To [2/2] : zye8hz
 * 8 | Fee : 3093 ATOM
 * 9 | Gas : 123705
 *
 * Normal Mode:
 *
 * 0 | Type : Send
 * 1 | Amount : 6000000 ATOM
 * 2 | From [1/2] : cosmos12fuhfs6juf3m47dgplrhdrkvdyn9wc2j
 * 2 | From [2/2] : mp4sw40
 * 3 | To [1/2] : cosmos1s3mu2eyc7ql5hrcqvuerfhxxfxs7ax4k
 * 3 | To [2/2] : zye8hz
 * 4 | Fee : 3093 ATOM
 *
 * Verification Details:
 * - Each item's content will be checked against the predefined values for the
 *   corresponding display index.
 * - If any discrepancy is found (either in item count or content), the function
 *   will return an error.
  * - The function will confirm that the number of items in the transaction
 *   matches the expected count for the current mode.
 */
parser_error_t parser_msg_send(parser_context_t *ctx_parsed_tx, uint8_t displayIdx, uint8_t pageIdx, uint8_t pageCount){
    if (ctx_parsed_tx == NULL) {
        return parser_unexpected_error;
    }
    char tmpKey[20] = {0};
    char tmpValue[65] = {0};
    int8_t chain_index = 0;

    // Check if app is in expert mode
    CHECK_PARSER_ERR(parser_getItem(ctx_parsed_tx, displayIdx, tmpKey, sizeof(tmpKey), tmpValue, sizeof(tmpValue), pageIdx, &pageCount))
    if (strcmp(tmpKey, "Chain ID") == 0) {
        chain_index = find_chain_index_by_chain_id(tmpValue);
        if (chain_index == -1) {
            ZEMU_LOGF(200, " Not supported Chain Id\n");
            return parser_swap_wrong_chain_id;
        }

        // If in expert mode check idx 3 for source address
        displayIdx += 3;
    }

    // Check source address is present
    CHECK_PARSER_ERR(parser_getItem(ctx_parsed_tx, displayIdx, tmpKey, sizeof(tmpKey), tmpValue, sizeof(tmpValue), pageIdx, &pageCount))
    if (strcmp(tmpKey, "Type") != 0 || strcmp(tmpValue, "Send") != 0) {
        return parser_swap_unexpected_field;
    }

    // Check amount
    displayIdx += 1;
    char tmp_amount[100] = {0};
    zxerr_t zxerr = format_amount(G_swap_state.amount, G_swap_state.amount_length, tmp_amount, sizeof(tmp_amount), chain_index);
    if (zxerr != zxerr_ok) {
        return parser_swap_wrap_amount_computation_error;
    }

    CHECK_PARSER_ERR(parser_getItem(ctx_parsed_tx, displayIdx, tmpKey, sizeof(tmpKey), tmpValue, sizeof(tmpValue), pageIdx, &pageCount))
    if (strcmp(tmpKey, "Amount") != 0 || strcmp(tmp_amount, tmpValue) != 0) {
        ZEMU_LOGF(200, "Wrong swap tx amount ('%s', should be : '%s').\n", tmpValue, tmp_amount);
        return parser_swap_wrong_amount;
    }

    displayIdx += 2;
    // Check destination address
    CHECK_PARSER_ERR(parser_getItem(ctx_parsed_tx, displayIdx, tmpKey, sizeof(tmpKey), tmpValue, sizeof(tmpValue), pageIdx, &pageCount))
    if (strcmp(tmpKey, "To") != 0 || strcmp(tmpValue, G_swap_state.destination_address) != 0) {
        ZEMU_LOGF(200, "Wrong swap tx destination address ('%s', should be : '%s').\n", tmpValue, G_swap_state.destination_address);
        return parser_swap_wrong_dest_address;
    }

    // Check if memo is present. If size of G_swap_state.memo is bigger than 0, then a memo should be present
    displayIdx += 1;
    uint8_t has_memo = 0;
    CHECK_PARSER_ERR(parser_getItem(ctx_parsed_tx, displayIdx, tmpKey, sizeof(tmpKey), tmpValue, sizeof(tmpValue), pageIdx, &pageCount))
    if (strlen(G_swap_state.memo) > 0) {
        if (strcmp(tmpKey, "Memo") == 0) {
            has_memo = 1;
            if(strcmp(tmpValue, G_swap_state.memo) != 0) {
                ZEMU_LOGF(200, "Wrong swap tx memo ('%s', should be : '%s').\n", tmpValue, G_swap_state.memo);
                return parser_swap_wrong_memo;
            }
        } else {
            return parser_swap_memo_not_present;
        }
        // Prepare for fees
        displayIdx += 1;   
        CHECK_PARSER_ERR(parser_getItem(ctx_parsed_tx, displayIdx, tmpKey, sizeof(tmpKey), tmpValue, sizeof(tmpValue), pageIdx, &pageCount))
    }

    //Check fees
    zxerr = format_amount(G_swap_state.fees, G_swap_state.fees_length, tmp_amount, sizeof(tmp_amount), chain_index);
    if (zxerr != zxerr_ok) {
        return parser_swap_wrap_amount_computation_error;
    }

    if (strcmp(tmpKey, "Fee") != 0 || strcmp(tmp_amount, tmpValue) != 0) {
        ZEMU_LOGF(200, "Wrong swap tx fees ('%s', should be : '%s').\n",  tmpValue, tmp_amount);
        return parser_swap_wrong_fee;
    }

    switch (has_memo) {
        case 0:
            // When there's no memo, expect one less item
            if (chain_index != 0 || app_mode_expert()) {
                if (ctx_parsed_tx->tx_obj->tx_json.num_items != EXPERT_SEND_MODE_ITEMS - 1) {
                    return parser_swap_unexpected_number_of_items;
                }
            } else {
                if (ctx_parsed_tx->tx_obj->tx_json.num_items != NORMAL_SEND_MODE_ITEMS - 1) {
                    return parser_swap_unexpected_number_of_items;
                }
            }
            break;
            
        case 1:
            // When there is a memo, expect full number of items
            if (chain_index != 0 || app_mode_expert()) {
                if (ctx_parsed_tx->tx_obj->tx_json.num_items != EXPERT_SEND_MODE_ITEMS) {
                    return parser_swap_unexpected_number_of_items;
                }
            } else {
                if (ctx_parsed_tx->tx_obj->tx_json.num_items != NORMAL_SEND_MODE_ITEMS) {
                    return parser_swap_unexpected_number_of_items;
                }
            }
            break;
            
        default:
            break;
    }

    return parser_ok;
}

/*
 * This function verifies that a received transaction follows the expected format
 * based on the current application mode (expert or normal). The verification
 * process includes checking the number of items in the transaction and validating
 * that the items at its respective display index matches the expected content.
 * If any item does not meet the expected criteria, the function will return an error.
 *
 * Expected transaction format:
 *
 * Expert Mode:
 *   0 | Chain ID        : cosmoshub-4
 *   1 | Account         : 0
 *   2 | Sequence        : 1
 *   3 | Source Address  : cosmosaccaddr1d9h8qat5e4ehc5
 *   4 | Source Coins    : 10 atom
 *   5 | Dest Address    : cosmosaccaddr1da6hgur4wse3jx32
 *   6 | Dest Coins      : 10 atom
 *   7 | Memo            : testmemo
 *   8 | Fee             : 5 photon
 *   9 | Gas             : 10000
 *
 * Normal Mode:
 *   0 | Source Address  : cosmosaccaddr1d9h8qat5e4ehc5
 *   1 | Source Coins    : 10 atom
 *   2 | Dest Address    : cosmosaccaddr1da6hgur4wse3jx32
 *   3 | Dest Coins      : 10 atom
 *   4 | Memo            : testmemo
 *   5 | Fee             : 5 photon
 *
 * Verification Details:
 * - Each item's content will be checked against the predefined values for the
 *   corresponding display index.
 * - If any discrepancy is found (either in item count or content), the function
 *   will return an error.
  * - The function will confirm that the number of items in the transaction
 *   matches the expected count for the current mode.
 */
parser_error_t parser_simple_transfer(parser_context_t *ctx_parsed_tx, uint8_t displayIdx, uint8_t pageIdx, uint8_t pageCount){
    if (ctx_parsed_tx == NULL) {
        return parser_unexpected_error;
    }

    char tmpKey[20] = {0};
    char tmpValue[65] = {0};
    int8_t chain_index = 0;
    // Check if app is in expert mode
    CHECK_PARSER_ERR(parser_getItem(ctx_parsed_tx, displayIdx, tmpKey, sizeof(tmpKey), tmpValue, sizeof(tmpValue), pageIdx, &pageCount))
    if (strcmp(tmpKey, "Chain ID") == 0) {
        chain_index = find_chain_index_by_chain_id(tmpValue);
        if (chain_index == -1) {
            ZEMU_LOGF(200, " Not supported Chain Id\n");
            return parser_swap_wrong_chain_id;
        }

        // If in expert mode check idx 3 for source address
        displayIdx += 3;
    }

    // Check source address is present
    CHECK_PARSER_ERR(parser_getItem(ctx_parsed_tx, displayIdx, tmpKey, sizeof(tmpKey), tmpValue, sizeof(tmpValue), pageIdx, &pageCount))
        if (strcmp(tmpKey, "Source Address") != 0) {
        return parser_swap_unexpected_field;
    }

    // Check source coins are equal to the amount and equal to destination coins
    char tmp_amount[100] = {0};
    zxerr_t zxerr = format_amount(G_swap_state.amount, G_swap_state.amount_length, tmp_amount, sizeof(tmp_amount), chain_index);
    if (zxerr != zxerr_ok) {
        return parser_swap_wrap_amount_computation_error;
    }

    displayIdx += 1;
    CHECK_PARSER_ERR(parser_getItem(ctx_parsed_tx, displayIdx, tmpKey, sizeof(tmpKey), tmpValue, sizeof(tmpValue), pageIdx, &pageCount))
    if (strcmp(tmpKey, "Source Coins") != 0 || strcmp(tmpValue, tmp_amount) != 0) {
        ZEMU_LOGF(200, "Wrong swap tx source coins ('%s', should be : '%s').\n", tmpValue, tmp_amount);
        return parser_swap_wrong_source_coins;
    }

    // Check destination address
    displayIdx += 1;
    CHECK_PARSER_ERR(parser_getItem(ctx_parsed_tx, displayIdx, tmpKey, sizeof(tmpKey), tmpValue, sizeof(tmpValue), pageIdx, &pageCount))
    if (strcmp(tmpKey, "Dest Address") != 0 || strcmp(tmpValue, G_swap_state.destination_address) != 0) {
        ZEMU_LOGF(200, "Wrong swap tx destination address ('%s', should be : '%s').\n", tmpValue, G_swap_state.destination_address);
        return parser_swap_wrong_dest_address;
    }

    // Check destination coins    
    displayIdx += 1;
    CHECK_PARSER_ERR(parser_getItem(ctx_parsed_tx, displayIdx, tmpKey, sizeof(tmpKey), tmpValue, sizeof(tmpValue), pageIdx, &pageCount))
    if (strcmp(tmpKey, "Dest Coins") != 0 || strcmp(tmp_amount, tmpValue) != 0) {
        ZEMU_LOGF(200, "Wrong swap tx destination coins ('%s', should be : '%s').\n", tmpValue, tmp_amount);
        return parser_swap_wrong_dest_coins;
    }

    // Check if memo is present. If size of G_swap_state.memo is bigger than 0, then a memo should be present
    displayIdx += 1;
    uint8_t has_memo = 0;
    CHECK_PARSER_ERR(parser_getItem(ctx_parsed_tx, displayIdx, tmpKey, sizeof(tmpKey), tmpValue, sizeof(tmpValue), pageIdx, &pageCount))
    if (strlen(G_swap_state.memo) > 0) {
        if (strcmp(tmpKey, "Memo") == 0) {
            has_memo = 1;
            if(strcmp(tmpValue, G_swap_state.memo) != 0) {
                ZEMU_LOGF(200, "Wrong swap tx memo ('%s', should be : '%s').\n", tmpValue, G_swap_state.memo);
                return parser_swap_wrong_memo;
            }
        } else {
            return parser_swap_memo_not_present;
        }
        // Prepare for fees
        displayIdx += 1;   
        CHECK_PARSER_ERR(parser_getItem(ctx_parsed_tx, displayIdx, tmpKey, sizeof(tmpKey), tmpValue, sizeof(tmpValue), pageIdx, &pageCount))
    }

    //Check fees
    zxerr = format_amount(G_swap_state.fees, G_swap_state.fees_length, tmp_amount, sizeof(tmp_amount), chain_index);
    if (zxerr != zxerr_ok) {
        return parser_swap_wrap_amount_computation_error;
    }

    if (strcmp(tmpKey, "Fee") != 0 || strcmp(tmp_amount, tmpValue) != 0) {
        ZEMU_LOGF(200, "Wrong swap tx fees ('%s', should be : '%s').\n",  tmpValue, tmp_amount);
        return parser_swap_wrong_fee;
    }

    switch (has_memo) {
        case 0:
            // When there's no memo, expect one less item
            if (chain_index != 0 || app_mode_expert()) {
                if (ctx_parsed_tx->tx_obj->tx_json.num_items != EXPERT_SEND_MODE_ITEMS - 1) {
                    return parser_swap_unexpected_number_of_items;
                }
            } else {
                if (ctx_parsed_tx->tx_obj->tx_json.num_items != NORMAL_SEND_MODE_ITEMS - 1) {
                    return parser_swap_unexpected_number_of_items;
                }
            }
            break;
            
        case 1:
            // When there is a memo, expect full number of items
            if (chain_index != 0 || app_mode_expert()) {
                if (ctx_parsed_tx->tx_obj->tx_json.num_items != EXPERT_SEND_MODE_ITEMS) {
                    return parser_swap_unexpected_number_of_items;
                }
            } else {
                if (ctx_parsed_tx->tx_obj->tx_json.num_items != NORMAL_SEND_MODE_ITEMS) {
                    return parser_swap_unexpected_number_of_items;
                }
            }
            break;
            
        default:
            break;
    }

    return parser_ok;
}

parser_error_t check_swap_conditions(parser_context_t *ctx_parsed_tx) {
    if (ctx_parsed_tx == NULL) {
        return parser_unexpected_error;
    }

    uint8_t displayIdx = 0;
    uint8_t pageIdx = 0;
    uint8_t pageCount = 0;

    // Check if the transaction is a simple transfer or a msg_send
    parser_error_t err = parser_simple_transfer(ctx_parsed_tx, displayIdx, pageIdx, pageCount);
    if (err == parser_swap_unexpected_field) {
        err = parser_msg_send(ctx_parsed_tx, displayIdx, pageIdx, pageCount);
    }

    return err;
}

void __attribute__((noreturn)) finalize_exchange_sign_transaction(bool is_success) {
    *G_swap_sign_return_value_address = is_success;
    os_lib_end();
}
