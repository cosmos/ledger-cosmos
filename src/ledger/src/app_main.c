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
                        if (view_scrolling_direction == 0) {
                            if (view_scrolling_step < (view_scrolling_step_count - 1)) {
                                view_scrolling_step++;
                            }
                            else {
                                view_scrolling_direction = 1;
                            }
                        }
                        else {
                            if (view_scrolling_step > 0) {
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

bool process_chunk(volatile uint32_t *tx, uint32_t rx)
{
    int packageIndex = G_io_apdu_buffer[OFFSET_PCK_INDEX];
    int packageCount = G_io_apdu_buffer[OFFSET_PCK_COUNT];

    if (packageIndex == 1) {
        transaction_reset();
    }

    transaction_append(&(G_io_apdu_buffer[OFFSET_DATA]), rx - OFFSET_DATA);

    return packageIndex == packageCount;
}

bool process_transaction(volatile uint32_t *tx, uint32_t rx)
{
    bool ready = process_chunk(tx, rx);

    if (ready) {
        transaction_parse();
    }

    return ready;
}

uint32_t getChecksum(uint8_t* buffer, unsigned int length)
{
    uint32_t checksum = 0;
    for (unsigned int i = 0; i < length; i++)
    {
        checksum += buffer[i];
    }
    return checksum;
}

void handleApdu(volatile uint32_t *flags, volatile uint32_t *tx, uint32_t rx)
{
    uint16_t sw = 0;

    BEGIN_TRY
    {
        TRY
        {
            if (G_io_apdu_buffer[OFFSET_CLA] != CLA) {
                THROW(APDU_CODE_CLA_NOT_SUPPORTED);
            }

            switch (G_io_apdu_buffer[OFFSET_INS]) {
                case INS_GET_VERSION:G_io_apdu_buffer[0] = 0x55;                     // CosmosApp
                    G_io_apdu_buffer[1] = LEDGER_MAJOR_VERSION;
                    G_io_apdu_buffer[2] = LEDGER_MINOR_VERSION;
                    G_io_apdu_buffer[3] = LEDGER_PATCH_VERSION;
                    *tx += 4;
                    THROW(APDU_CODE_OK);
                    break;

                case INS_SIGN:
                    if (process_transaction(tx, rx)) {
                        view_display_transaction_menu(transaction_get_info(NULL, NULL, -1));

                        *flags |= IO_ASYNCH_REPLY;
                    }
                    else {
                        THROW(APDU_CODE_OK);
                    }
                    break;

                case INS_HASH:
                    if (process_chunk(tx, rx)) {
                        uint8_t message_digest[CX_SHA256_SIZE];

                        cx_hash_sha256(transaction_get_buffer(),
                                       transaction_get_buffer_length(),
                                       message_digest,
                                       CX_SHA256_SIZE);

                        os_memmove(G_io_apdu_buffer, message_digest, CX_SHA256_SIZE);
                        *tx += 32;
                    }
                    THROW(APDU_CODE_OK);
                    break;

                case INS_GET_PUBLIC_KEY:
                    generate_public_key_from_bip();

                    os_memmove(G_io_apdu_buffer,
                               get_response_buffer(),
                               get_response_buffer_length());
                    *tx += get_response_buffer_length();
                    THROW(APDU_CODE_OK);
                    break;

                case INS_ECHO:
                    if (process_chunk(tx, rx)) {

                        uint32_t checksum = getChecksum(transaction_get_buffer(), transaction_get_buffer_length());

                        G_io_apdu_buffer[0] = (checksum >> 24) & 255;
                        G_io_apdu_buffer[1] = (checksum >> 16) & 255;
                        G_io_apdu_buffer[2] = (checksum >> 8) & 255;
                        G_io_apdu_buffer[3] = (checksum & 255);
                        *tx += 4;
                    }
                    THROW(APDU_CODE_OK);
                    break;

                case INS_GET_PUBLIC_KEY_DUMMY:
                {
                    // Start with hardcoded private key
                    uint8_t privateKeyData[32];
                    memset(privateKeyData, 0, 32);

                    // Generate public key
                    cx_ecfp_public_key_t publicKey;
                    cx_ecfp_private_key_t privateKey;

                    cx_ecfp_init_private_key(
                            CX_CURVE_256K1,
                            privateKeyData,
                            32,
                            &privateKey);

                    cx_ecfp_init_public_key(CX_CURVE_256R1, NULL, 0, &publicKey);
                    cx_ecfp_generate_pair(CX_CURVE_256K1, &publicKey, &privateKey, 1);

                    os_memmove(G_io_apdu_buffer, publicKey.W, 65);
                    *tx += 65;

                    THROW(APDU_CODE_OK);
                }
                    break;

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
            G_io_apdu_buffer[*tx + 1] = sw;
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
    // TODO: Signature could be in the stack and moved to the apdu_buffer
    int valid = generate_signature_SECP256K1(transaction_get_buffer(), transaction_get_buffer_length());

    //valid = 1;

    if (valid) {
        uint8_t *signature = get_response_buffer();
        uint32_t length = get_response_buffer_length();
        os_memmove(G_io_apdu_buffer, signature, length);

        set_code(G_io_apdu_buffer, length, APDU_CODE_OK);
        io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, length + 2);
        view_display_signing_success();
    }
    else {
        set_code(G_io_apdu_buffer, 0, APDU_CODE_UNKNOWN);
        io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
        view_display_signing_error();
    }
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

                if (rx == 0) THROW(APDU_CODE_EMPTY_BUFFER);

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
                G_io_apdu_buffer[tx + 1] = sw;
                tx += 2;
            }
            FINALLY;
            {}
        }
        END_TRY;
    }
}

