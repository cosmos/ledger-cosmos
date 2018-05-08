/*******************************************************************************
*   (c) 2016 Ledger
*   (c) 2018 ZondaX GmbH
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
#include "view.h"
#include "transaction.h"
#include "signature.h"

#include <os_io_seproxyhal.h>
#include <os.h>

#include <string.h>

#ifdef TESTING_ENABLED
// Generate using always the same private data
// to allow for reproducible results
const uint8_t privateKeyDataTest[] = {
        0x75, 0x56, 0x0e, 0x4d, 0xde, 0xa0, 0x63, 0x05,
        0xc3, 0x6e, 0x2e, 0xb5, 0xf7, 0x2a, 0xca, 0x71,
        0x2d, 0x13, 0x4c, 0xc2, 0xa0, 0x59, 0xbf, 0xe8,
        0x7e, 0x9b, 0x5d, 0x55, 0xbf, 0x81, 0x3b, 0xd4
};
#endif

uint8_t bip32_depth;
uint32_t bip32_path[10];
unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];

unsigned char io_event(unsigned char channel)
{
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

    case SEPROXYHAL_TAG_TICKER_EVENT:   //
        UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer, {
                if (UX_ALLOWED) {
                    if (view_scrolling_step_count) {
                        // prepare next screen
                        if (view_scrolling_direction==0) {
                            if (view_scrolling_step<(view_scrolling_step_count-1)) {
                                view_scrolling_step++;
                            }
                            else {
                                view_scrolling_direction = 1;
                            }
                        }
                        else {
                            if (view_scrolling_step>0) {
                                view_scrolling_step--;
                            }
                            else {
                                view_scrolling_direction = 0;
                            }
                        }
                        // redisplay screen
                        UX_REDISPLAY();
                    }
                }
        });
        break;

        // unknown events are acknowledged
    default:UX_DEFAULT_EVENT();
        break;
    }
    if (!io_seproxyhal_spi_is_status_sent()) {
        io_seproxyhal_general_status();
    }
    return 1; // DO NOT reset the current APDU transport
}

unsigned short io_exchange_al(unsigned char channel, unsigned short tx_len)
{
    switch (channel & ~(IO_FLAGS)) {
    case CHANNEL_KEYBOARD:break;

        // multiplexed io exchange over a SPI channel and TLV encapsulated protocol
    case CHANNEL_SPI:
        if (tx_len) {
            io_seproxyhal_spi_send(G_io_apdu_buffer, tx_len);

            if (channel & IO_RESET_AFTER_REPLIED) {
                reset();
            }
            return 0; // nothing received from the master so far (it's a tx
            // transaction)
        }
        else {
            return io_seproxyhal_spi_recv(G_io_apdu_buffer,
                    sizeof(G_io_apdu_buffer), 0);
        }

    default:THROW(INVALID_PARAMETER);
    }
    return 0;
}

void app_init()
{
    io_seproxyhal_init();
    USB_power(0);
    USB_power(1);
    view_idle(0);
}

bool process_chunk(volatile uint32_t* tx, uint32_t rx, bool getBip32)
{
    int packageIndex = G_io_apdu_buffer[OFFSET_PCK_INDEX];
    int packageCount = G_io_apdu_buffer[OFFSET_PCK_COUNT];

    uint16_t offset = OFFSET_DATA;
    if (rx<offset) {
        THROW(APDU_CODE_DATA_INVALID);
    }

    if (packageIndex==1) {
        transaction_reset();
        if (getBip32) {
            if (!extractBip32(&bip32_depth, bip32_path, rx, 4)) {
                THROW(APDU_CODE_DATA_INVALID);
            }
        }
    }
    else {
        transaction_append(&(G_io_apdu_buffer[offset]), rx-offset);
    }

    return packageIndex==packageCount;
}

bool extractBip32(uint8_t* depth, uint32_t path[10], uint32_t rx, uint32_t offset)
{
    if (rx<offset+1) {
        return 0;
    }

    *depth = G_io_apdu_buffer[offset];
    const uint16_t req_offset = 4**depth+1+offset;

    if (rx<req_offset || *depth>10) {
        return 0;
    }
    memcpy(path, G_io_apdu_buffer+offset+1, *depth*4);
    return 1;
}

void handleApdu(volatile uint32_t* flags, volatile uint32_t* tx, uint32_t rx)
{
    uint16_t sw = 0;

    BEGIN_TRY
    {
        TRY
        {
            if (G_io_apdu_buffer[OFFSET_CLA]!=CLA) {
                THROW(APDU_CODE_CLA_NOT_SUPPORTED);
            }

            switch (G_io_apdu_buffer[OFFSET_INS]) {
            case INS_GET_VERSION: {
#ifdef TESTING_ENABLED
                G_io_apdu_buffer[0] = 0xFF;
#else
                G_io_apdu_buffer[0] = 0x55;
#endif
                G_io_apdu_buffer[1] = LEDGER_MAJOR_VERSION;
                G_io_apdu_buffer[2] = LEDGER_MINOR_VERSION;
                G_io_apdu_buffer[3] = LEDGER_PATCH_VERSION;
                *tx += 4;
                THROW(APDU_CODE_OK);
                break;
            }

            case INS_PUBLIC_KEY: {
                if (!extractBip32(&bip32_depth, bip32_path, rx, 2)) {
                    THROW(APDU_CODE_DATA_INVALID);
                }

                cx_ecfp_public_key_t publicKey;
                cx_ecfp_private_key_t privateKey;
                uint8_t privateKeyData[32];

                // Generate keys
                os_perso_derive_node_bip32(
                        CX_CURVE_256K1,
                        bip32_path, bip32_depth,
                        privateKeyData, NULL);
                keys_secp256k1(&publicKey, &privateKey, privateKeyData);
                memset(privateKeyData, 0, 32);

                os_memmove(G_io_apdu_buffer, publicKey.W, 65);
                *tx += 65;

                THROW(APDU_CODE_OK);
            }

            case INS_SIGN: {
                if (!process_chunk(tx, rx, true))
                    THROW(APDU_CODE_OK);

                transaction_parse();
                view_display_transaction_menu(transaction_get_info(NULL, NULL, -1));

                *flags |= IO_ASYNCH_REPLY;
            }
                break;

#ifdef TESTING_ENABLED
                case INS_HASH_TEST: {
                    if (process_chunk(tx, rx, false)) {
                        uint8_t message_digest[CX_SHA256_SIZE];

                        cx_hash_sha256(transaction_get_buffer(),
                                       transaction_get_buffer_length(),
                                       message_digest,
                                       CX_SHA256_SIZE);

                        os_memmove(G_io_apdu_buffer, message_digest, CX_SHA256_SIZE);
                        *tx += 32;
                    }
                    THROW(APDU_CODE_OK);
                }
                break;

                case INS_SIGN_TEST: {
                    if (process_chunk(tx, rx, false)) {

                        unsigned int length = 0;

                        // Generate keys
                        cx_ecfp_public_key_t publicKey;
                        cx_ecfp_private_key_t privateKey;
                        keys_secp256k1(&publicKey, &privateKey, privateKeyDataTest );

                        // Skip UI and validation
                        sign_secp256k1(
                                transaction_get_buffer(),
                                transaction_get_buffer_length(),
                                G_io_apdu_buffer,
                                IO_APDU_BUFFER_SIZE,
                                &length,
                                &privateKey);

                        *tx += length;
                    }
                    THROW(APDU_CODE_OK);
                }
                break;

                case INS_PUBLIC_KEY_TEST: {
                    // Generate key
                    cx_ecfp_public_key_t publicKey;
                    cx_ecfp_private_key_t privateKey;
                    keys_secp256k1(&publicKey, &privateKey, privateKeyDataTest );

                    os_memmove(G_io_apdu_buffer, publicKey.W, 65);
                    *tx += 65;

                    THROW(APDU_CODE_OK);
                }
                break;
#endif

            default:THROW(APDU_CODE_INS_NOT_SUPPORTED);
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
            case APDU_CODE_OK:sw = e;
                break;
            default:sw = 0x6800 | (e & 0x7FF);
                break;
            }
            G_io_apdu_buffer[*tx] = sw >> 8;
            G_io_apdu_buffer[*tx+1] = sw;
            *tx += 2;
        }
        FINALLY
        {
        }
    }
    END_TRY;
}

void reject_transaction()
{
    set_code(G_io_apdu_buffer, 0, APDU_CODE_COMMAND_NOT_ALLOWED);
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
    view_idle(0);
}

void sign_transaction()
{
    // Generate keys
    cx_ecfp_public_key_t publicKey;
    cx_ecfp_private_key_t privateKey;
    uint8_t privateKeyData[32];

//    const uint32_t bip32_derivation_path[] =
//            {
//                    0x80000000 | 44,
//                    0x80000000 | 60,
//                    0x80000000 | 0,
//                    0x80000000 | 0,
//                    0x80000000 | 0
//            };
//    bip32_private(bip32_derivation_path, 5, privateKeyData);

    os_perso_derive_node_bip32(
            CX_CURVE_256K1,
            bip32_path, bip32_depth,
            privateKeyData, NULL);

    keys_secp256k1(&publicKey, &privateKey, privateKeyData);
    memset(privateKeyData, 0, 32);

    unsigned int length = 0;
    sign_secp256k1(
            transaction_get_buffer(),
            transaction_get_buffer_length(),
            G_io_apdu_buffer,
            IO_APDU_BUFFER_SIZE,
            &length,
            &privateKey);

    set_code(G_io_apdu_buffer, length, APDU_CODE_OK);
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, length+2);
    view_display_signing_success();
}

void app_main()
{
    volatile uint32_t rx = 0, tx = 0, flags = 0;

    view_add_update_transaction_info_event_handler(&transaction_get_info);
    view_add_reject_transaction_event_handler(&reject_transaction);
    view_add_sign_transaction_event_handler(&sign_transaction);

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

                if (rx==0) THROW(APDU_CODE_EMPTY_BUFFER);

                handleApdu(&flags, &tx, rx);
            }
            CATCH_OTHER(e);
            {
                switch (e & 0xF000) {
                case 0x6000:
                case 0x9000:sw = e;
                    break;
                default:sw = 0x6800 | (e & 0x7FF);
                    break;
                }
                G_io_apdu_buffer[tx] = sw >> 8;
                G_io_apdu_buffer[tx+1] = sw;
                tx += 2;
            }
            FINALLY;
            { }
        }
        END_TRY;
    }
}

