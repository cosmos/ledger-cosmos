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
#include "crypto.h"
#include "lib_standard_app/swap_lib_calls.h"
#include "lib_standard_app/bip32.h"
#include "swap.h"
#include "zxformat.h"
#include "swap_utils.h"
#include "chain_config.h"

/*
Address parameters have the following structure;
    - bip32 path length (1 byte) | bip32 path (4 * pathLength bytes)
Coin Configuration has the following structure;
    - hrp length (1 byte) | hrp (hrp length bytes)
*/
void handle_check_address(check_address_parameters_t *params) {
    if (params == NULL || params->address_to_check == NULL) {
        return;
    }

    // Reset result
    params->result = 0;

    // Get HRP
    if (params->coin_configuration == NULL) {
        ZEMU_LOGF(200, "Coin configuration is NULL\n");
        return;
    }

    uint8_t hrp_length = params->coin_configuration[0];
    ZEMU_LOGF(200, "HRP length: %d\n", hrp_length);
    char hrp[MAX_BECH32_HRP_LEN + 1] = {0};

    if (hrp_length == 0 || hrp_length > MAX_BECH32_HRP_LEN) {
        return;
    }
    memcpy(hrp, params->coin_configuration + 1, hrp_length);
    hrp[hrp_length] = 0;

    // Get bip32 path
    uint8_t bip32_path_length = params->address_parameters[0];
    uint32_t bip32_path[HDPATH_LEN_DEFAULT] = {0};

    if (bip32_path_length != HDPATH_LEN_DEFAULT) {
        return;
    }

    for (uint32_t i = 0; i < HDPATH_LEN_DEFAULT; i++) {
        readU32BE(params->address_parameters + 1 + (i * 4), &bip32_path[i]);
    }

    // Check if the chain is supported with the HRP and path
    address_encoding_e encode_type = checkChainConfig(bip32_path[1], hrp, hrp_length);
    if (encode_type == UNSUPPORTED) {
        return;
    }

    char address_computed[100] = {0};
    uint16_t reply_len = 0;
    zxerr_t err = crypto_swap_fillAddress(bip32_path, bip32_path_length, hrp, encode_type, address_computed, 
                                            sizeof(address_computed), &reply_len);
    if (err != zxerr_ok) {
        MEMZERO(address_computed, sizeof(address_computed));
        return;
    }

    // Exchange guarantees that the input string is '\0' terminated
    uint8_t address_to_check_len = strlen(params->address_to_check);

    if (reply_len == address_to_check_len && memcmp(address_computed, params->address_to_check, reply_len) == 0) {
        params->result = 1;
    }
}
