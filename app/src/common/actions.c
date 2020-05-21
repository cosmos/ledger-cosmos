/*******************************************************************************
*   (c) 2016 Ledger
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

#include "actions.h"
#include "crypto.h"
#include "tx.h"
#include "apdu_codes.h"
#include <os_io_seproxyhal.h>
#include "coin.h"

uint8_t app_sign() {
    uint8_t *signature = G_io_apdu_buffer;

    const uint8_t *message = tx_get_buffer();
    const uint16_t messageLength = tx_get_buffer_length();

    return crypto_sign(signature, IO_APDU_BUFFER_SIZE - 2, message, messageLength);
}

void app_set_hrp(char *p) {
    crypto_set_hrp(p);
}

uint8_t app_fill_address() {
    // Put data directly in the apdu buffer
    MEMZERO(G_io_apdu_buffer, IO_APDU_BUFFER_SIZE);
    return crypto_fillAddress(G_io_apdu_buffer, IO_APDU_BUFFER_SIZE - 2);
}

void app_reply_address() {
    const uint8_t replyLen = app_fill_address();
    set_code(G_io_apdu_buffer, replyLen, APDU_CODE_OK);
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, replyLen + 2);
}

void app_reply_error() {
    set_code(G_io_apdu_buffer, 0, APDU_CODE_DATA_INVALID);
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
}
