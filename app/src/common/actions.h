/*******************************************************************************
*   (c) 2019 Zondax GmbH
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
#include "crypto.h"
#include "tx.h"
#include "apdu_codes.h"
#include <os_io_seproxyhal.h>
#include "coin.h"

void app_sign();

void app_set_hrp(char *p);

extern uint8_t action_addr_len;

__Z_INLINE uint8_t app_fill_address(address_kind_e kind) {
    // Put data directly in the apdu buffer
    MEMZERO(G_io_apdu_buffer, IO_APDU_BUFFER_SIZE);

    switch (kind) {
        case addr_secp256k1:
            action_addr_len = crypto_fillAddress(G_io_apdu_buffer, IO_APDU_BUFFER_SIZE - 2);
            break;
        default:
            action_addr_len = 0;
            break;
    }

    return action_addr_len;
}

__Z_INLINE void app_reply_address(address_kind_e kind) {
    UNUSED(kind);

    set_code(G_io_apdu_buffer, action_addr_len, APDU_CODE_OK);
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, action_addr_len + 2);
}

__Z_INLINE void app_reply_error() {
    set_code(G_io_apdu_buffer, 0, APDU_CODE_DATA_INVALID);
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
}
