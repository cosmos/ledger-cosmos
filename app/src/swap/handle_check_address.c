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


void handle_check_address(check_address_parameters_t *params) {
    if (params == NULL || params->address_to_check == 0) {
        return;
    }

    // Reset result
    params->result = 0;

    // Address parameters have the following structure
    // path length (1 byte) | bip32 path (4 * pathLength bytes)
    // Get the path
    uint32_t bip32_path[HDPATH_LEN_DEFAULT] = {0};
    uint8_t bip32_path_length = params->address_parameters[0];

    if (bip32_path_length != HDPATH_LEN_DEFAULT) {
        return;
    }

    for (uint32_t i = 0; i < HDPATH_LEN_DEFAULT; i++) {
        readU32BE(params->address_parameters + 2 + (i * 4), &bip32_path[i]);
    }

    char address_computed[100] = {0};
    uint16_t reply_len = 0;
    zxerr_t err = crypto_swap_fillAddress(bip32_path,
                                                bip32_path_length, address_computed, sizeof(address_computed),
                                                &reply_len);
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
