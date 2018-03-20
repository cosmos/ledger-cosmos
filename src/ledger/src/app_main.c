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

#include <os_io_seproxyhal.h>
#include "os.h"
#include "ui.h"
#include "app_main.h"

parsed_json_t parsed_json;
char json_buffer[JSON_BUFFER_SIZE];
uint32_t json_buffer_write_pos;
uint32_t json_buffer_size;

#if SEND_STACK_INFORMATION
volatile uint32_t stackStartAddress;
#endif

// Updates text of the title and value parameters
// based on the information from the parsed_json.
// parsed_json is split into displayable elements
// in this order:
// - for each input:
//      - input address
//      - for each input coin
//          - coin name
//          - coin amount
// - for each output
//      - output address
//      - for each output coin
//          - coin name
//          - coin amount
int update_transaction_ui_data (char* title,
                                int titleSize,
                                char* value,
                                int valueSize,
                                int currentPage)
{
    int pageIndex = 0;
    for (int i=0; i < parsed_json.NumberOfInputs; i++)
    {
        if (currentPage == pageIndex) {
            os_memmove((char *) title, "Input address", sizeof("Input address"));

            int addressSize =   parsed_json.Tokens[parsed_json.Inputs[i].Address].end -
                                parsed_json.Tokens[parsed_json.Inputs[i].Address].start;
            const char* addressPtr = json_buffer + parsed_json.Tokens[parsed_json.Inputs[i].Address].start;
            os_memmove((char *) value, addressPtr, addressSize);
            value[addressSize] = '\0';
            return pageIndex;
        }
        pageIndex++;
        for (int j=0; j < parsed_json.Inputs[i].NumberOfCoins; j++)
        {
            if (currentPage == pageIndex) {
                os_memmove((char *) title, "Coin", sizeof("Coin"));

                int coinSize =  parsed_json.Tokens[parsed_json.Inputs[i].Coins[j].Denum].end -
                                parsed_json.Tokens[parsed_json.Inputs[i].Coins[j].Denum].start;
                const char* coinPtr = json_buffer + parsed_json.Tokens[parsed_json.Inputs[i].Coins[j].Denum].start;
                os_memmove((char *) value, coinPtr, coinSize);
                value[coinSize] = '\0';
                return pageIndex;
            }
            pageIndex++;
            if (currentPage == pageIndex) {
                os_memmove((char *) title, "Amount", sizeof("Amount"));

                int coinAmountSize =    parsed_json.Tokens[parsed_json.Inputs[i].Coins[j].Amount].end -
                                        parsed_json.Tokens[parsed_json.Inputs[i].Coins[j].Amount].start;
                const char* coinAmountPtr = json_buffer + parsed_json.Tokens[parsed_json.Inputs[i].Coins[j].Amount].start;

                os_memmove((char *) value, coinAmountPtr, coinAmountSize);
                value[coinAmountSize] = '\0';
                return pageIndex;
            }
            pageIndex++;
        }
    }
    for (int i=0; i < parsed_json.NumberOfOutputs; i++)
    {
        if (currentPage == pageIndex) {
            os_memmove((char *) title, "Output address", sizeof("Output address"));

            int addressSize =   parsed_json.Tokens[parsed_json.Outputs[i].Address].end -
                                parsed_json.Tokens[parsed_json.Outputs[i].Address].start;
            const char* addressPtr = json_buffer + parsed_json.Tokens[parsed_json.Outputs[i].Address].start;
            os_memmove((char *) value, addressPtr, addressSize);
            value[addressSize] = '\0';
            return pageIndex;
        }
        pageIndex++;
        for (int j=0; j < parsed_json.Outputs[i].NumberOfCoins; j++)
        {
            if (currentPage == pageIndex) {
                os_memmove((char *) title, "Coin", sizeof("Coin"));

                int coinSize =  parsed_json.Tokens[parsed_json.Outputs[i].Coins[j].Denum].end -
                                parsed_json.Tokens[parsed_json.Outputs[i].Coins[j].Denum].start;
                const char* coinPtr = json_buffer + parsed_json.Tokens[parsed_json.Outputs[i].Coins[j].Denum].start;
                os_memmove((char *) value, coinPtr, coinSize);
                value[coinSize] = '\0';
                return pageIndex;
            }
            pageIndex++;
            if (currentPage == pageIndex) {
                os_memmove((char *) title, "Amount", sizeof("Amount"));

                int coinAmountSize =    parsed_json.Tokens[parsed_json.Outputs[i].Coins[j].Amount].end -
                                        parsed_json.Tokens[parsed_json.Outputs[i].Coins[j].Amount].start;
                const char* coinAmountPtr = json_buffer + parsed_json.Tokens[parsed_json.Outputs[i].Coins[j].Amount].start;

                os_memmove((char *) value, coinAmountPtr, coinAmountSize);
                value[coinAmountSize] = '\0';
                return pageIndex;
            }
            pageIndex++;
        }
    }
    return pageIndex;
}

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
//            if ((uiState == UI_TEXT) &&
//                (os_seph_features() &
//                 SEPROXYHAL_TAG_SESSION_START_EVENT_FEATURE_SCREEN_BIG)) {
//                if (!display_text_part()) {
//                    ui_approval();
//                } else {
//                    UX_REDISPLAY();
//                }
//            } else {
//                UX_DISPLAYED_EVENT();
//            }
//            break;

            if (!UX_DISPLAYED())
                UX_DISPLAYED_EVENT();
            break;

        case SEPROXYHAL_TAG_TICKER_EVENT:   //
            break;

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

void app_init()
{
    io_seproxyhal_init();
    USB_power(0);
    USB_power(1);
    ui_idle(0);
}

bool process_json(volatile uint32_t *tx, uint32_t rx)
{
    int packageIndex = G_io_apdu_buffer[OFFSET_PCK_INDEX];
    int packageCount = G_io_apdu_buffer[OFFSET_PCK_COUNT];

    if (packageIndex == 1) {
        json_buffer_write_pos = 0;
    }
    os_memmove(json_buffer + json_buffer_write_pos, &(G_io_apdu_buffer[OFFSET_DATA]), rx - OFFSET_DATA + 1);
    json_buffer_write_pos += (rx - OFFSET_DATA + 1);
    json_buffer [json_buffer_write_pos] = '\0';

    int position = 0;

    if (packageIndex != packageCount) {
        G_io_apdu_buffer[*tx + position++] = packageIndex;
        G_io_apdu_buffer[*tx + position++] = packageCount;
        G_io_apdu_buffer[*tx + position++] = (rx - OFFSET_DATA + 1);
    }
    else
    {
        os_memset((void *) &parsed_json, 0, sizeof(parsed_json_t));
        ParseJson(&parsed_json, json_buffer);

        G_io_apdu_buffer[*tx + position++] = packageIndex;
        G_io_apdu_buffer[*tx + position++] = packageCount;
        G_io_apdu_buffer[*tx + position++] = json_buffer_write_pos;

        G_io_apdu_buffer[*tx + position++] = parsed_json.NumberOfInputs;
        G_io_apdu_buffer[*tx + position++] = parsed_json.NumberOfOutputs;
        G_io_apdu_buffer[*tx + position++] = parsed_json.NumberOfTokens;
        G_io_apdu_buffer[*tx + position++] = rx;

#if SEND_STACK_INFORMATION
        G_io_apdu_buffer[*tx + position++] = (stackStartAddress & 0xFF000000) >> 24;
        G_io_apdu_buffer[*tx + position++] = (stackStartAddress & 0x00FF0000) >> 16;
        G_io_apdu_buffer[*tx + position++] = (stackStartAddress & 0x0000FF00) >> 8;
        G_io_apdu_buffer[*tx + position++] = (stackStartAddress & 0x000000FF);

        int data = 0;
        uint32_t currentStackAddress = (uint32_t) & data;
        G_io_apdu_buffer[*tx + position++] = (currentStackAddress & 0xFF000000) >> 24;
        G_io_apdu_buffer[*tx + position++] = (currentStackAddress & 0x00FF0000) >> 16;
        G_io_apdu_buffer[*tx + position++] = (currentStackAddress & 0x0000FF00) >> 8;
        G_io_apdu_buffer[*tx + position++] = (currentStackAddress & 0x000000FF);
#endif
    }
    *tx += position;

    return packageIndex == packageCount;
}

void handleApdu(volatile uint32_t *flags, volatile uint32_t *tx, uint32_t rx) {
    uint16_t sw = 0;

    BEGIN_TRY {
        TRY
        {
            if (G_io_apdu_buffer[OFFSET_CLA] != CLA) {
                THROW(0x6E00);
            }

            switch (G_io_apdu_buffer[OFFSET_INS]) {
                case 1:
                    if (process_json(tx, rx))
                    {
                        ui_display_transaction(update_transaction_ui_data(0, 0, 0, 0, -1));
                        *flags |= IO_ASYNCH_REPLY;
                    }
                    else {
                        THROW(0x9000);
                    }
                    break;

                default:
                    THROW(0x9000);
            }
        }
        CATCH(EXCEPTION_IO_RESET) {
            THROW(EXCEPTION_IO_RESET);
        }
        CATCH_OTHER(e) {
            switch (e & 0xF000) {
                case 0x6000:
                case 0x9000:
                    sw = e;
                    break;
                default:
                    sw = 0x6800 | (e & 0x7FF);
                    break;
            }

            G_io_apdu_buffer[*tx] = sw >> 8;
            G_io_apdu_buffer[*tx+1] = sw;

            *tx += 2;
        }
        FINALLY {
        }
    }
    END_TRY;
}

void reject_transaction()
{
    ui_idle(0);
    THROW(0x9000);
}

void app_main() {
    volatile uint32_t rx = 0, tx = 0, flags = 0;

    json_buffer_write_pos = 0;
    json_buffer_size = 0;

#if SEND_STACK_INFORMATION
    stackStartAddress = (uint32_t)&rx;
#endif
    set_update_transaction_ui_data_callback(&update_transaction_ui_data);
    set_reject_transaction_callback(&reject_transaction);

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

                if (rx == 0) THROW(0x6982);

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

