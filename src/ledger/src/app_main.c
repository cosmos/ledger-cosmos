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


volatile uint32_t stackStartAddress = 0;
volatile uint32_t maxUsedStackSize = 0;
parsed_json_t parsed_json;

void update_stack_info()
{
    int dummyData = 0;
    if (stackStartAddress == 0) {
        stackStartAddress = (uint32_t)&dummyData;
    }
    else {
        uint32_t currentUsedStackSize = (uint32_t)&dummyData - stackStartAddress;
        if (currentUsedStackSize > maxUsedStackSize) {
            maxUsedStackSize = currentUsedStackSize;
        }
    }
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
    ui_idle();
}

void process_json(volatile uint32_t *tx, uint32_t rx)
{
    update_stack_info();

    G_io_apdu_buffer[rx - 1] = '\0';
    char* msg = (char*)&(G_io_apdu_buffer[OFFSET_INS + 1]);

    os_memset((void*)&parsed_json, 0, sizeof(parsed_json_t));
    ParseJson(&parsed_json, msg);
    update_stack_info();

    //const char* sendMsgSample = "{\"_df\":\"3CAAA78D13BAE0\",\"_v\":{\"inputs\":[{\"address\":\"696E707574\",\"coins\":[{\"denom\":\"atom\",\"amount\":10}],\"sequence\":1}],\"outputs\":[{\"address\":\"6F7574707574\",\"coins\":[{\"denom\":\"atom\",\"amount\":10}]}]}}";
    // ParseJson(&parsedMessage, sendMsgSample);

    int position = 0;
    G_io_apdu_buffer[*tx+position++] = parsed_json.NumberOfInputs;
    G_io_apdu_buffer[*tx+position++] = parsed_json.NumberOfOutputs;
    G_io_apdu_buffer[*tx+position++] = parsed_json.NumberOfTokens;
    G_io_apdu_buffer[*tx+position++] = rx;

    G_io_apdu_buffer[*tx+position++] = maxUsedStackSize & 0xFF000000 >> 24;
    G_io_apdu_buffer[*tx+position++] = maxUsedStackSize & 0x00FF0000 >> 16;
    G_io_apdu_buffer[*tx+position++] = maxUsedStackSize & 0x0000FF00 >> 8;
    G_io_apdu_buffer[*tx+position++] = maxUsedStackSize & 0x000000FF;

    *tx += position;

    //memcpy(input_address,
    //       msg + parsedMessage.Tokens[parsedMessage.Inputs[0].Address].start,
    //       parsedMessage.Tokens[parsedMessage.Inputs[0].Address].end - parsedMessage.Tokens[parsedMessage.Inputs[0].Address].start);

    //UX_DISPLAY(bagl_ui_input_address, NULL);
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
                    process_json(tx, rx);
                    THROW(0x9000);
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

void app_main() {
    volatile uint32_t rx = 0, tx = 0, flags = 0;

    update_stack_info();

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

