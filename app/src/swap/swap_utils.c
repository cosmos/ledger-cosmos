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
#include "swap.h"
#include "zxformat.h"
#include "app_mode.h"

const chains_t chains[] = {
    {COIN_DEFAULT_CHAINID, " ATOM", " uatom", 6, "cosmos"},
    {OSMOSIS_CHAINID, " OSMO", " uosmo", 6, "osmo"},
    {DYDX_CHAINID, " DYDX", " adydx", 18, "dydx"},
    {MANTRA_CHAINID, " OM", " uom", 18, "mantra"},
    {XION_CHAINID, " XION", " uxion", 18, "xion"},
};

const uint32_t chains_len = sizeof(chains) / sizeof(chains[0]);

int8_t find_chain_index_by_coin_config(const char *coin_config, uint8_t coin_config_len) {
    if (coin_config == NULL) {
        return -1;
    }

    for (uint8_t i = 0; i < chains_len; i++) {
        if (strncmp(coin_config, PIC(chains[i].coin_config), coin_config_len) == 0) {
            return i;
        }
    }
    
    return -1;
}

int8_t find_chain_index_by_chain_id(const char *chain_id) {
    if (chain_id == NULL) {
        return -1;
    }

    for (uint8_t i = 0; i < chains_len; i++) {
        if (strncmp(chain_id, PIC(chains[i].chain_id), strlen(PIC(chains[i].chain_id))) == 0) {
            return i;
        }
    }

    return -1;
}

zxerr_t bytesAmountToStringBalance(uint8_t *amount, uint8_t amount_len, char *out, uint8_t out_len, int8_t chain_index) {
    uint8_t tmpBuf[COIN_AMOUNT_MAXSIZE] = {0};

    bignumBigEndian_to_bcd(tmpBuf, sizeof(tmpBuf), amount, amount_len);
    bignumBigEndian_bcdprint(out, out_len, tmpBuf, sizeof(tmpBuf));

    // Format number.
    if (!intstr_to_fpstr_inplace(out, out_len, chains[chain_index].decimals)) {
        return zxerr_encoding_failed;
    }

    // Add ticker prefix.
    CHECK_ZXERR(z_str3join(out, out_len, "", PIC(chains[chain_index].ticker)))

    // Trim trailing zeros
    number_inplace_trimming(out, 1);

    return zxerr_ok;
}

zxerr_t bytesAmountToExpertStringBalance(uint8_t *amount, uint8_t amount_len, char *out, uint8_t out_len, int8_t chain_index) {
    uint8_t tmpBuf[COIN_AMOUNT_MAXSIZE] = {0};

    bignumBigEndian_to_bcd(tmpBuf, sizeof(tmpBuf), amount, amount_len);
    bignumBigEndian_bcdprint(out, out_len, tmpBuf, sizeof(tmpBuf));

    // Add expert ticker prefix
    CHECK_ZXERR(z_str3join(out, out_len, "", PIC(chains[chain_index].expert_ticker)))

    return zxerr_ok;
}

zxerr_t format_amount(uint8_t *amount, uint8_t amount_len, char *out, uint8_t out_len, int8_t chain_index) {
    // expert or not default chain
    if (app_mode_expert() || chain_index != 0) {
        return bytesAmountToExpertStringBalance(amount, amount_len, out, out_len, chain_index);
    } else {
        return bytesAmountToStringBalance(amount, amount_len, out, out_len, chain_index);
    }
}

zxerr_t readU32BE(uint8_t *input, uint32_t *output) {
    if (input == NULL || output == NULL) {
        return zxerr_no_data;
    }

    *output = 0;
    for (uint8_t i = 0; i < 4; i++) {
        *output += (uint32_t) * (input + i) << (32 - (8 * (i + 1)));
    }
    return zxerr_ok;
}
