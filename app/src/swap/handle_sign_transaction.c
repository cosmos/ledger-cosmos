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
 * - The function will first confirm that the number of items in the transaction
 *   matches the expected count for the current mode.
 * - Each item's content will be checked against the predefined values for the
 *   corresponding display index.
 * - If any discrepancy is found (either in item count or content), the function
 *   will return an error.
 */
parser_error_t check_swap_conditions(parser_context_t *ctx_parsed_tx) {
    parser_error_t err = parser_unexpected_error;
    if (ctx_parsed_tx == NULL) {
        return err;
    }

    uint8_t displayIdx = 0;
    uint8_t pageIdx = 0;
    uint8_t pageCount = 0;
    char tmpKey[20] = {0};
    char tmpValue[65] = {0};

    if ((app_mode_expert() && ctx_parsed_tx->tx_obj->tx_json.num_items != EXPERT_MODE_ITEMS) || (!app_mode_expert() && ctx_parsed_tx->tx_obj->tx_json.num_items != NORMAL_MODE_ITEMS)) {
        return parser_unexpected_error;
    }

    // Cosmos App in normal mode requires that chain id is the default one. If not, it will print expert mode fields
    // this means if we reach this point and no chain_id is printed, chain_id must be the default one
    const char *default_chain_id = "cosmoshub-4";
    CHECK_PARSER_ERR(parser_getItem(ctx_parsed_tx, displayIdx, tmpKey, sizeof(tmpKey), tmpValue, sizeof(tmpValue), pageIdx, &pageCount))

    // Check if chain_id is printed, expert fields
    if (strcmp(tmpKey, "Chain ID") == 0) {
        // For now allow only default chain id
        if (strcmp(tmpValue, default_chain_id) != 0) {
            ZEMU_LOGF(200, "Wrong Chain Id. ('%s', should be : '%s').\n", tmpValue, default_chain_id);
            return parser_unexpected_error;
        }
        displayIdx += 5; // skip account_number, sequence, source_address, source_coins
    } else {
        displayIdx += 2; // skipsource_address, source_coins
    }

    // Check destination address
    CHECK_PARSER_ERR(parser_getItem(ctx_parsed_tx, displayIdx, tmpKey, sizeof(tmpKey), tmpValue, sizeof(tmpValue), pageIdx, &pageCount))
    if (strcmp(tmpKey, "Dest Address") != 0 || strcmp(tmpValue, G_swap_state.destination_address) != 0) {
        ZEMU_LOGF(200, "Wrong swap tx destination address ('%s', should be : '%s').\n", tmpValue, G_swap_state.destination_address);
        return parser_unexpected_error;
    }

    // Check destination coins
    char tmp_amount[100] = {0};
    zxerr_t zxerr = bytesAmountToStringBalance(G_swap_state.amount, G_swap_state.amount_length, tmp_amount, sizeof(tmp_amount));
    if (zxerr != zxerr_ok) {
        return parser_unexpected_error;
    }
    
    displayIdx += 1;
    CHECK_PARSER_ERR(parser_getItem(ctx_parsed_tx, displayIdx, tmpKey, sizeof(tmpKey), tmpValue, sizeof(tmpValue), pageIdx, &pageCount))
    if (strcmp(tmpKey, "Dest Coins") != 0 || strcmp(tmp_amount, tmpValue) != 0) {
        ZEMU_LOGF(200, "Wrong swap tx destination coins ('%s', should be : '%s').\n", tmpValue, tmp_amount);
        return parser_unexpected_error;
    }

    // Check if memo is present
    displayIdx += 1;
    CHECK_PARSER_ERR(parser_getItem(ctx_parsed_tx, displayIdx, tmpKey, sizeof(tmpKey), tmpValue, sizeof(tmpValue), pageIdx, &pageCount))
    if (strcmp(tmpKey, "Memo") == 0) {
        if(strcmp(tmpValue, G_swap_state.memo) != 0) {
            ZEMU_LOGF(200, "Wrong swap tx memo ('%s', should be : '%s').\n", tmpValue, G_swap_state.memo);
            return parser_unexpected_error;
        }

        // Prepare for fees
        displayIdx += 1;   
        CHECK_PARSER_ERR(parser_getItem(ctx_parsed_tx, displayIdx, tmpKey, sizeof(tmpKey), tmpValue, sizeof(tmpValue), pageIdx, &pageCount))
    }

    //Check fees
    zxerr = bytesAmountToStringBalance(G_swap_state.fees, G_swap_state.fees_length, tmp_amount, sizeof(tmp_amount));
    if (zxerr != zxerr_ok) {
        return parser_unexpected_error;
    }

    if (strcmp(tmpKey, "Fee") != 0 || strcmp(tmp_amount, tmpValue) != 0) {
        ZEMU_LOGF(200, "Wrong swap tx fees ('%s', should be : '%s').\n",  tmpValue, tmp_amount);
        return parser_unexpected_error;
    }

    return parser_ok;
}

void __attribute__((noreturn)) finalize_exchange_sign_transaction(bool is_success) {
    *G_swap_sign_return_value_address = is_success;
    os_lib_end();
}
