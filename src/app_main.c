/*******************************************************************************
*   (c) 2018, 2019 ZondaX GmbH
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

#include "view.h"
#include "actions.h"
#include "tx.h"
#include "lib/crypto.h"
#include "cosmos.h"
#include "zxmacros.h"

unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];

unsigned char io_event(unsigned char channel) {
    switch (G_io_seproxyhal_spi_buffer[0]) {
        case SEPROXYHAL_TAG_FINGER_EVENT: //
            UX_FINGER_EVENT(G_io_seproxyhal_spi_buffer);
            break;

        case SEPROXYHAL_TAG_BUTTON_PUSH_EVENT: // for Nano S
            UX_BUTTON_PUSH_EVENT(G_io_seproxyhal_spi_buffer);
            break;

        case SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT:
            if (!UX_DISPLAYED())
                UX_DISPLAYED_EVENT();
            break;

        case SEPROXYHAL_TAG_TICKER_EVENT: { //
            UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer, {
                if (UX_ALLOWED) {
                    UX_REDISPLAY();
                }
            });
            break;
        }

            // unknown events are acknowledged
        default:
            UX_DEFAULT_EVENT();
            break;
    }
    if (!io_seproxyhal_spi_is_status_sent()) {
        io_seproxyhal_general_status();
    }
    return 1; // DO NOT reset the current APDU transport
}

unsigned short io_exchange_al(unsigned char channel, unsigned short tx_len) {
    switch (channel & ~(IO_FLAGS)) {
        case CHANNEL_KEYBOARD:
            break;

            // multiplexed io exchange over a SPI channel and TLV encapsulated protocol
        case CHANNEL_SPI:
            if (tx_len) {
                io_seproxyhal_spi_send(G_io_apdu_buffer, tx_len);

                if (channel & IO_RESET_AFTER_REPLIED) {
                    reset();
                }
                return 0; // nothing received from the master so far (it's a tx
                // transaction)
            } else {
                return io_seproxyhal_spi_recv(G_io_apdu_buffer, sizeof(G_io_apdu_buffer), 0);
            }

        default:
            THROW(INVALID_PARAMETER);
    }
    return 0;
}

void extractBip32(uint32_t rx, uint32_t offset) {
    if ((rx - offset) < 1 + 4 * BIP32_LEN_DEFAULT) {
        THROW(APDU_CODE_DATA_INVALID);
    }

    uint8_t depth = G_io_apdu_buffer[offset];
    if (depth != BIP32_LEN_DEFAULT) {
        THROW(APDU_CODE_DATA_INVALID);
    }

    uint8_t *p = (uint8_t * )(G_io_apdu_buffer + offset + 1);
    memcpy(bip32Path, p, 4 * BIP32_LEN_DEFAULT);

    if (bip32Path[0] != BIP32_0_DEFAULT ||
        bip32Path[1] != BIP32_1_DEFAULT ||
        bip32Path[3] != BIP32_3_DEFAULT) {
        THROW(APDU_CODE_DATA_INVALID);
    }
}

bool process_chunk(volatile uint32_t *tx, uint32_t rx) {
    int packageIndex = G_io_apdu_buffer[OFFSET_PCK_INDEX];
    int packageCount = G_io_apdu_buffer[OFFSET_PCK_COUNT];

    uint16_t offset = OFFSET_DATA;
    if (rx < offset) {
        THROW(APDU_CODE_DATA_INVALID);
    }

    if (packageIndex == 1) {
        tx_initialize();
        tx_reset();

        extractBip32(rx, OFFSET_DATA);

        return packageIndex == packageCount;
    }

    if (tx_append(&(G_io_apdu_buffer[offset]), rx - offset) != rx - offset) {
        THROW(APDU_CODE_OUTPUT_BUFFER_TOO_SMALL);
    }

    return packageIndex == packageCount;
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
#ifdef TESTING_ENABLED
                    G_io_apdu_buffer[0] = 0xFF;
#else
                    G_io_apdu_buffer[0] = 0;
#endif
                    G_io_apdu_buffer[1] = LEDGER_MAJOR_VERSION;
                    G_io_apdu_buffer[2] = LEDGER_MINOR_VERSION;
                    G_io_apdu_buffer[3] = LEDGER_PATCH_VERSION;
                    G_io_apdu_buffer[4] = !IS_UX_ALLOWED;

                    G_io_apdu_buffer[5] = (TARGET_ID >> 24) & 0xFF;
                    G_io_apdu_buffer[6] = (TARGET_ID >> 16) & 0xFF;
                    G_io_apdu_buffer[7] = (TARGET_ID >> 8) & 0xFF;
                    G_io_apdu_buffer[8] = (TARGET_ID >> 0) & 0xFF;

                    *tx += 9;
                    THROW(APDU_CODE_OK);
                    break;
                }

                case INS_GET_ADDR_SECP256K1: {
                    uint8_t len = extractHRP(rx, OFFSET_DATA);
                    extractBip32(rx, OFFSET_DATA + 1 + len);

                    uint8_t requireConfirmation = G_io_apdu_buffer[OFFSET_P1];

                    if (requireConfirmation) {
                        app_fill_address();
                        view_address_show();
                        *flags |= IO_ASYNCH_REPLY;
                        break;
                    }

                    *tx = app_fill_address();
                    THROW(APDU_CODE_OK);
                    break;
                }

                case INS_SIGN_SECP256K1: {
                    if (!process_chunk(tx, rx))
                        THROW(APDU_CODE_OK);

                    const char *error_msg = tx_parse();

                    if (error_msg != NULL) {
                        int error_msg_length = strlen(error_msg);
                        os_memmove(G_io_apdu_buffer, error_msg, error_msg_length);
                        *tx += (error_msg_length);
                        THROW(APDU_CODE_BAD_KEY_HANDLE);
                    }

                    view_sign_show();
                    *flags |= IO_ASYNCH_REPLY;
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

void handle_generic_apdu(volatile uint32_t *flags, volatile uint32_t *tx, uint32_t rx) {
    if (rx > 4 && os_memcmp(G_io_apdu_buffer, "\xE0\x01\x00\x00", 4) == 0) {
        // Respond to get device info command
        uint8_t *p = G_io_apdu_buffer;
        // Target ID        4 bytes
        p[0] = (TARGET_ID >> 24) & 0xFF;
        p[1] = (TARGET_ID >> 16) & 0xFF;
        p[2] = (TARGET_ID >> 8) & 0xFF;
        p[3] = (TARGET_ID >> 0) & 0xFF;
        p += 4;
        // SE Version       [length][non-terminated string]
        *p = os_version(p + 1, 64);
        p = p + 1 + *p;
        // Flags            [length][flags]
        *p = 0;
        p++;
        // MCU Version      [length][non-terminated string]
        *p = os_seph_version(p + 1, 64);
        p = p + 1 + *p;

        *tx = p - G_io_apdu_buffer;
        THROW(APDU_CODE_OK);
    }
}

void app_init() {
    io_seproxyhal_init();
    USB_power(0);
    USB_power(1);
    view_idle_show(0);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

void app_main() {
    volatile uint32_t rx = 0, tx = 0, flags = 0;

    for (;;) {
        volatile uint16_t sw = 0;

        BEGIN_TRY;
        {
            TRY;
            {
                rx = tx;
                tx = 0;
                rx = io_exchange(CHANNEL_APDU | flags, rx);
                flags = 0;

                if (rx == 0)
                    THROW(APDU_CODE_EMPTY_BUFFER);

                handle_generic_apdu(&flags, &tx, rx);

                handleApdu(&flags, &tx, rx);
            }
            CATCH_OTHER(e);
            {
                switch (e & 0xF000) {
                    case 0x6000:
                    case 0x9000:
                        sw = e;
                        break;
                    default:
                        sw = 0x6800 | (e & 0x7FF);
                        break;
                }
                G_io_apdu_buffer[tx] = sw >> 8;
                G_io_apdu_buffer[tx + 1] = sw;
                tx += 2;
            }
            FINALLY;
            {}
        }
        END_TRY;
    }
}

#pragma clang diagnostic pop
