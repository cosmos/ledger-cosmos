/*******************************************************************************
*   (c) 2018 - 2023 Zondax AG
*   (c) 2016 Ledger
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

#include "app_main.h"

#include <string.h>
#include <os_io_seproxyhal.h>
#include <os.h>
#include <ux.h>

#include "view.h"
#include "actions.h"
#include "tx.h"
#include "addr.h"
#include "crypto.h"
#include "coin.h"
#include "zxmacros.h"
#include "app_mode.h"

#include "parser_impl.h"
#include "view_internal.h"

#include "chain_config.h"

static const char *msg_error1 = "Expert Mode";
static const char *msg_error2 = "Required";

__Z_INLINE void handle_getversion(__Z_UNUSED volatile uint32_t *flags, volatile uint32_t *tx, __Z_UNUSED uint32_t rx) {
#ifdef DEBUG
    G_io_apdu_buffer[0] = 0xFF;
#else
    G_io_apdu_buffer[0] = 0;
#endif
    G_io_apdu_buffer[1] = LEDGER_MAJOR_VERSION;
    G_io_apdu_buffer[2] = LEDGER_MINOR_VERSION;
    G_io_apdu_buffer[3] = LEDGER_PATCH_VERSION;
    // SDK won't let the app reply an apdu message if screensaver is active
    G_io_apdu_buffer[4] = true;

    G_io_apdu_buffer[5] = (TARGET_ID >> 24) & 0xFF;
    G_io_apdu_buffer[6] = (TARGET_ID >> 16) & 0xFF;
    G_io_apdu_buffer[7] = (TARGET_ID >> 8) & 0xFF;
    G_io_apdu_buffer[8] = (TARGET_ID >> 0) & 0xFF;

    *tx += 9;
    THROW(APDU_CODE_OK);
}

static void extractHDPath(uint32_t rx, uint32_t offset) {
    if ((rx - offset) < sizeof(uint32_t) * HDPATH_LEN_DEFAULT) {
        THROW(APDU_CODE_WRONG_LENGTH);
    }

    MEMCPY(hdPath, G_io_apdu_buffer + offset, sizeof(uint32_t) * HDPATH_LEN_DEFAULT);

    // Check values
    if (hdPath[0] != HDPATH_0_DEFAULT ||
        ((hdPath[1] != HDPATH_1_DEFAULT) && (hdPath[1] != HDPATH_ETH_1_DEFAULT)) ||
        hdPath[3] != HDPATH_3_DEFAULT) {
        THROW(APDU_CODE_DATA_INVALID);
    }

    encoding = checkChainConfig(hdPath[1], bech32_hrp, bech32_hrp_len);
    if (encoding == UNSUPPORTED) {
        ZEMU_LOGF(50, "Chain config not supported for: %s\n", bech32_hrp)
        THROW(APDU_CODE_COMMAND_NOT_ALLOWED);
    }

    // Limit values unless the app is running in expert mode
    if (!app_mode_expert()) {
        for(int i=2; i < HDPATH_LEN_DEFAULT; i++) {
            // hardened or unhardened values should be below 20
            if ( (hdPath[i] & 0x7FFFFFFF) > 100) THROW(APDU_CODE_CONDITIONS_NOT_SATISFIED);
        }
    }
}

static bool process_chunk(volatile uint32_t *tx, uint32_t rx) {
    UNUSED(tx);

    const uint8_t payloadType = G_io_apdu_buffer[OFFSET_PAYLOAD_TYPE];

    if (G_io_apdu_buffer[OFFSET_P2] != 0) {
        THROW(APDU_CODE_INVALIDP1P2);
    }

    if (rx < OFFSET_DATA) {
        THROW(APDU_CODE_WRONG_LENGTH);
    }

    uint32_t added;
    switch (payloadType) {
        case P1_INIT:
            tx_initialize();
            tx_reset();
            extractHDPath(rx, OFFSET_DATA);
            return false;
        case P1_ADD:
            added = tx_append(&(G_io_apdu_buffer[OFFSET_DATA]), rx - OFFSET_DATA);
            if (added != rx - OFFSET_DATA) {
                THROW(APDU_CODE_OUTPUT_BUFFER_TOO_SMALL);
            }
            return false;
        case P1_LAST:
            added = tx_append(&(G_io_apdu_buffer[OFFSET_DATA]), rx - OFFSET_DATA);
            if (added != rx - OFFSET_DATA) {
                THROW(APDU_CODE_OUTPUT_BUFFER_TOO_SMALL);
            }
            return true;
    }

    THROW(APDU_CODE_INVALIDP1P2);
}

__Z_INLINE void handleGetAddrSecp256K1(volatile uint32_t *flags, volatile uint32_t *tx, uint32_t rx) {
    uint8_t len = extractHRP(rx, OFFSET_DATA);
    extractHDPath(rx, OFFSET_DATA + 1 + len);

    uint8_t requireConfirmation = G_io_apdu_buffer[OFFSET_P1];
    zxerr_t zxerr = app_fill_address();
    if (zxerr != zxerr_ok) {
        *tx = 0;
        THROW(APDU_CODE_DATA_INVALID);
    }

    if (requireConfirmation) {
        view_review_init(addr_getItem, addr_getNumItems, app_reply_address);
        view_review_show(REVIEW_ADDRESS);
        *flags |= IO_ASYNCH_REPLY;
        return;
    }

    *tx = action_addrResponseLen;
    THROW(APDU_CODE_OK);
}

__Z_INLINE void handleSignSecp256K1(volatile uint32_t *flags, volatile uint32_t *tx, uint32_t rx) {
    if (!process_chunk(tx, rx)) {
        THROW(APDU_CODE_OK);
    }

    // Put address in output buffer, we will use it to confirm source address
    zxerr_t zxerr = app_fill_address();
    if (zxerr != zxerr_ok) {
        *tx = 0;
        THROW(APDU_CODE_DATA_INVALID);
    }
    parser_tx_obj.own_addr = (const char *) (G_io_apdu_buffer + VIEW_ADDRESS_OFFSET_SECP256K1);

    if ((encoding != BECH32_COSMOS) && !app_mode_expert()) {
        *flags |= IO_ASYNCH_REPLY;
        view_custom_error_show(PIC(msg_error1),PIC(msg_error2));
        THROW(APDU_CODE_DATA_INVALID);
    }
    const char *error_msg = tx_parse();

    if (error_msg != NULL) {
        int error_msg_length = strlen(error_msg);
        MEMCPY(G_io_apdu_buffer, error_msg, error_msg_length);
        *tx += (error_msg_length);
        THROW(APDU_CODE_DATA_INVALID);
    }

    CHECK_APP_CANARY()
    view_review_init(tx_getItem, tx_getNumItems, app_sign);
    view_review_show(REVIEW_TXN);
    *flags |= IO_ASYNCH_REPLY;
}

void handleApdu(volatile uint32_t *flags, volatile uint32_t *tx, uint32_t rx) {
    uint16_t sw = 0;

    BEGIN_TRY
    {
        TRY
        {
            if (G_io_apdu_buffer[OFFSET_CLA] != CLA) {
                THROW(APDU_CODE_CLA_NOT_SUPPORTED);
            }

            if (rx < APDU_MIN_LENGTH) {
                THROW(APDU_CODE_WRONG_LENGTH);
            }

            switch (G_io_apdu_buffer[OFFSET_INS]) {
                case INS_GET_VERSION: {
                    handle_getversion(flags, tx, rx);
                    break;
                }

                case INS_GET_ADDR_SECP256K1: {
                    CHECK_PIN_VALIDATED()
                    handleGetAddrSecp256K1(flags, tx, rx);
                    break;
                }

                case INS_SIGN_SECP256K1: {
                    CHECK_PIN_VALIDATED()
                    handleSignSecp256K1(flags, tx, rx);
                    break;
                }

                default:
                    THROW(APDU_CODE_INS_NOT_SUPPORTED);
            }
        }
        CATCH(EXCEPTION_IO_RESET)
        {
            THROW(EXCEPTION_IO_RESET);
        }
        CATCH_OTHER(e)
        {
            switch (e & 0xF000) {
                case 0x6000:
                case APDU_CODE_OK:
                    sw = e;
                    break;
                default:
                    sw = 0x6800 | (e & 0x7FF);
                    break;
            }
            G_io_apdu_buffer[*tx] = sw >> 8;
            G_io_apdu_buffer[*tx + 1] = sw;
            *tx += 2;
        }
        FINALLY
        {
        }
    }
    END_TRY;
}
