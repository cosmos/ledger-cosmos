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
#include "lib/transaction.h"
#include "signature.h"
#include "zxmacros.h"
#include "bech32.h"

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
sigtype_t current_sigtype;

char bech32_hrp[MAX_BECH32_HRP_LEN + 1];
uint8_t bech32_hrp_len;

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
                return io_seproxyhal_spi_recv(G_io_apdu_buffer,
                                              sizeof(G_io_apdu_buffer), 0);
            }

        default:
            THROW(INVALID_PARAMETER);
    }
    return 0;
}

void app_init() {
    io_seproxyhal_init();
    USB_power(0);
    USB_power(1);
    view_idle(0);
}

bool extractBip32(uint8_t *depth, uint32_t path[10], uint32_t rx, uint32_t offset) {
    if (rx < offset + 1) {
        return 0;
    }

    *depth = G_io_apdu_buffer[offset];
    const uint16_t req_offset = 4 * *depth + 1 + offset;

    if (rx < req_offset || *depth > 10) {
        return 0;
    }

    memcpy(path, G_io_apdu_buffer + offset + 1, *depth * 4);
    return 1;
}

bool validateCosmosPath(uint8_t depth, uint32_t path[10]) {
    // Only paths in the form 44'/118'/{account}'/0/{index} are supported
    if (bip32_depth != 5) {
        return 0;
    }
    if (path[0] != 0x8000002c || path[1] != 0x80000076 || path[3] != 0) {
        return 0;
    }
    return 1;
}

bool extractHRP(uint8_t *len, char *hrp, uint32_t rx, uint32_t offset) {
    if (rx < offset + 1) {
        THROW(APDU_CODE_DATA_INVALID);
    }

    *len = G_io_apdu_buffer[offset];

    if (*len == 0 || *len > MAX_BECH32_HRP_LEN) {
        THROW(APDU_CODE_DATA_INVALID);
    }

    memcpy(hrp, G_io_apdu_buffer + offset + 1, *len);
    hrp[*len] = 0; // zero terminate
    return 1;
}

void getPubKey(cx_ecfp_public_key_t *publicKey) {
    cx_ecfp_private_key_t privateKey;
    uint8_t privateKeyData[32];

    // Generate keys
    os_perso_derive_node_bip32(
            CX_CURVE_256K1,
            bip32_path, bip32_depth,
            privateKeyData, NULL);
    keys_secp256k1(publicKey, &privateKey, privateKeyData);
    memset(privateKeyData, 0, sizeof(privateKeyData));
    memset(&privateKey, 0, sizeof(privateKey));
}

bool process_chunk(volatile uint32_t *tx, uint32_t rx, bool getBip32) {
    int packageIndex = G_io_apdu_buffer[OFFSET_PCK_INDEX];
    int packageCount = G_io_apdu_buffer[OFFSET_PCK_COUNT];

    uint16_t offset = OFFSET_DATA;
    if (rx < offset) {
        THROW(APDU_CODE_DATA_INVALID);
    }

    if (packageIndex == 1) {
        transaction_initialize();
        transaction_reset();
        if (getBip32) {
            if (!extractBip32(&bip32_depth, bip32_path, rx, OFFSET_DATA)) {
                THROW(APDU_CODE_DATA_INVALID);
            }
            return packageIndex == packageCount;
        }
    }

    if (transaction_append(&(G_io_apdu_buffer[offset]), rx - offset) != rx - offset) {
        THROW(APDU_CODE_OUTPUT_BUFFER_TOO_SMALL);
    }

    return packageIndex == packageCount;
}

//region View Transaction Handlers

int tx_getData(
        char *title, int max_title_length,
        char *key, int max_key_length,
        char *value, int max_value_length,
        int page_index,
        int chunk_index,
        int *page_count_out,
        int *chunk_count_out) {

    *page_count_out = transaction_get_display_pages();

    switch (current_sigtype) {
        case SECP256K1:
            snprintf(title, max_title_length, "SECP256K1 %02d/%02d", page_index + 1, *page_count_out);
            break;
        default:
            snprintf(title, max_title_length, "INVALID!");
            break;
    }

    // The API is different so we need to temporarily send chunk_index => chunk_count_out
    int16_t tmp = chunk_index;
    transaction_get_display_key_value(
            key, max_key_length,
            value, max_value_length,
            page_index,
            &tmp);

    *chunk_count_out = tmp;
    return 0;
}

void tx_accept_sign() {
    // Generate keys
    cx_ecfp_public_key_t publicKey;
    cx_ecfp_private_key_t privateKey;
    uint8_t privateKeyData[32];

    unsigned int length = 0;
    int result = 0;
    switch (current_sigtype) {
        case SECP256K1:
            os_perso_derive_node_bip32(
                    CX_CURVE_256K1,
                    bip32_path, bip32_depth,
                    privateKeyData, NULL);

            keys_secp256k1(&publicKey, &privateKey, privateKeyData);
            memset(privateKeyData, 0, 32);

            result = sign_secp256k1(
                    transaction_get_buffer(),
                    transaction_get_buffer_length(),
                    G_io_apdu_buffer,
                    IO_APDU_BUFFER_SIZE,
                    &length,
                    &privateKey);
            break;
        default:
            THROW(APDU_CODE_INS_NOT_SUPPORTED);
            break;
    }
    if (result == 1) {
        set_code(G_io_apdu_buffer, length, APDU_CODE_OK);
        io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, length + 2);
        view_display_signing_success();
    } else {
        set_code(G_io_apdu_buffer, length, APDU_CODE_SIGN_VERIFY_ERROR);
        io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, length + 2);
        view_display_signing_error();
    }
}

void tx_reject() {
    set_code(G_io_apdu_buffer, 0, APDU_CODE_COMMAND_NOT_ALLOWED);
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
    view_idle(0);
}

//endregion

//region View Address Handlers

void ripemd160_32(uint8_t *out, uint8_t *in) {
    cx_ripemd160_t rip160;
    cx_ripemd160_init(&rip160);
    cx_hash(&rip160.header, CX_LAST, in, CX_SHA256_SIZE, out, CX_RIPEMD160_SIZE);
}

#define PK_COMPRESSED_LEN 33

void get_pk_compressed(uint8_t *pkc) {
    cx_ecfp_public_key_t publicKey;
    // Modify the last part of the path
    getPubKey(&publicKey);
    // "Compress" public key in place
    publicKey.W[0] = publicKey.W[64] & 1 ? 0x03 : 0x02;
    memcpy(pkc, publicKey.W, PK_COMPRESSED_LEN);
}

int addr_getData(char *title, int max_title_length,
                 char *key, int max_key_length,
                 char *value, int max_value_length,
                 int page_index,
                 int chunk_index,
                 int *page_count_out,
                 int *chunk_count_out) {

    *page_count_out = 0x7FFFFFFF;
    *chunk_count_out = 1;

    snprintf(title, max_title_length, "Account %d", bip32_path[2] & 0x7FFFFFF);
    snprintf(key, max_key_length, "index %d", page_index);

    bip32_path[bip32_depth - 1] = page_index;
    uint8_t tmp[PK_COMPRESSED_LEN];
    get_pk_compressed(tmp);

    // Convert pubkey to address
    uint8_t hashed_pk[CX_RIPEMD160_SIZE];
    cx_hash_sha256(tmp, PK_COMPRESSED_LEN, tmp, CX_SHA256_SIZE);
    ripemd160_32(hashed_pk, tmp);

    // Convert address to bech32
    bech32EncodeFromBytes(value, bech32_hrp, hashed_pk, CX_RIPEMD160_SIZE);

    return 0;
}

void addr_accept() {
    int pos = 0;
    // Send pubkey
    get_pk_compressed(G_io_apdu_buffer + pos);
    pos += PK_COMPRESSED_LEN;

    // Send bech32 addr
    strcpy((char *) (G_io_apdu_buffer + pos), (char *)viewctl_DataValue);
    pos += strlen((char *)viewctl_DataValue);

    set_code(G_io_apdu_buffer, pos, APDU_CODE_OK);
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, pos + 2);
    view_idle(0);
}

void addr_reject() {
    set_code(G_io_apdu_buffer, 0, APDU_CODE_COMMAND_NOT_ALLOWED);
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
    view_idle(0);
}

//endregion

void handleApdu(volatile uint32_t *flags, volatile uint32_t *tx, uint32_t rx) {
    uint16_t sw = 0;

    BEGIN_TRY
    {
        TRY
        {
            if (G_io_apdu_buffer[OFFSET_CLA] != CLA) {
                THROW(APDU_CODE_CLA_NOT_SUPPORTED);
            }

            if (rx < 5) {
                THROW(APDU_CODE_WRONG_LENGTH);
            }

            switch (G_io_apdu_buffer[OFFSET_INS]) {
                case INS_GET_VERSION: {
                    unsigned int UX_ALLOWED = (ux.params.len != BOLOS_UX_IGNORE && ux.params.len != BOLOS_UX_CONTINUE);

#ifdef TESTING_ENABLED
                    G_io_apdu_buffer[0] = 0xFF;
#else
                    G_io_apdu_buffer[0] = 0;
#endif
                    G_io_apdu_buffer[1] = LEDGER_MAJOR_VERSION;
                    G_io_apdu_buffer[2] = LEDGER_MINOR_VERSION;
                    G_io_apdu_buffer[3] = LEDGER_PATCH_VERSION;
                    G_io_apdu_buffer[4] = !UX_ALLOWED;

                    *tx += 5;
                    THROW(APDU_CODE_OK);
                    break;
                }

                // INS_PUBLIC_KEY_SECP256K1 will be deprecated in the near future
                case INS_PUBLIC_KEY_SECP256K1: {
                    if (!extractBip32(&bip32_depth, bip32_path, rx, OFFSET_DATA)) {
                        THROW(APDU_CODE_DATA_INVALID);
                    }

                    if (!validateCosmosPath(bip32_depth, bip32_path)) {
                        THROW(APDU_CODE_DATA_INVALID);
                    }

                    cx_ecfp_public_key_t publicKey;
                    getPubKey(&publicKey);

                    os_memmove(G_io_apdu_buffer, publicKey.W, 65);
                    *tx += 65;

                    THROW(APDU_CODE_OK);
                    break;
                }

                case INS_SHOW_ADDR_SECP256K1: {
                    // Parse arguments
                    if (!extractHRP(&bech32_hrp_len, bech32_hrp, rx, OFFSET_DATA)) {
                        THROW(APDU_CODE_DATA_INVALID);
                    }

                    if (!extractBip32(&bip32_depth, bip32_path, rx, OFFSET_DATA + bech32_hrp_len + 1)) {
                        THROW(APDU_CODE_DATA_INVALID);
                    }

                    if (!validateCosmosPath(bip32_depth, bip32_path)) {
                        THROW(APDU_CODE_DATA_INVALID);
                    }

                    view_set_handlers(addr_getData, NULL, NULL);
                    view_addr_show(bip32_path[4] & 0x7FFFFFF);

                    *flags |= IO_ASYNCH_REPLY;
                    break;
                }

                case INS_GET_ADDR_SECP256K1: {
                    // Parse arguments
                    if (!extractHRP(&bech32_hrp_len, bech32_hrp, rx, OFFSET_DATA)) {
                        THROW(APDU_CODE_DATA_INVALID);
                    }

                    if (!extractBip32(&bip32_depth, bip32_path, rx, OFFSET_DATA + bech32_hrp_len + 1)) {
                        THROW(APDU_CODE_DATA_INVALID);
                    }

                    if (!validateCosmosPath(bip32_depth, bip32_path)) {
                        THROW(APDU_CODE_DATA_INVALID);
                    }

                    view_set_handlers(addr_getData, addr_accept, addr_reject);
                    view_addr_confirm(bip32_path[4] & 0x7FFFFFF);

                    *flags |= IO_ASYNCH_REPLY;
                    break;
                }

                case INS_SIGN_SECP256K1: {
                    current_sigtype = SECP256K1;
                    if (!process_chunk(tx, rx, true))
                        THROW(APDU_CODE_OK);

                    const char *error_msg = transaction_parse();
                    if (error_msg != NULL) {
                        int error_msg_length = strlen(error_msg);
                        os_memmove(G_io_apdu_buffer, error_msg, error_msg_length);
                        *tx += (error_msg_length);
                        THROW(APDU_CODE_BAD_KEY_HANDLE);
                    }

                    view_set_handlers(tx_getData, tx_accept_sign, tx_reject);
                    view_tx_show(0);

                    *flags |= IO_ASYNCH_REPLY;
                    break;
                }

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

                case INS_PUBLIC_KEY_SECP256K1_TEST: {
                    // Generate key
                    cx_ecfp_public_key_t publicKey;
                    cx_ecfp_private_key_t privateKey;
                    keys_secp256k1(&publicKey, &privateKey, privateKeyDataTest );

                    os_memmove(G_io_apdu_buffer, publicKey.W, 65);
                    *tx += 65;

                    THROW(APDU_CODE_OK);
                }
                break;

                case INS_SIGN_SECP256K1_TEST: {
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
#endif

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
