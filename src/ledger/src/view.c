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

#include "view.h"
#include "view_templates.h"
#include "common.h"

#include "glyphs.h"
#include "bagl.h"

#include <string.h>
#include <stdio.h>

ux_state_t ux;
enum UI_STATE view_uiState;

void update_transaction_page_info();

// Current scrolling position of view msg
unsigned short view_scrolling_step = 0;
// Maximum number of characters to scroll view msg (view_scrolling_total_size - screen size)
unsigned short view_scrolling_step_count = 0;
// Total size of the view message
unsigned short view_scrolling_total_size = 0;
// Direction of the view msg scroll (0 - left to right, 1 - right to left)
unsigned char view_scrolling_direction = 0;

// Current scrolling position of key msg
unsigned short key_scrolling_step = 0;
// Maximum number of characters to scroll key msg (view_scrolling_total_size - screen size)
unsigned short key_scrolling_step_count = 0;
// Total size of the key message
unsigned short key_scrolling_total_size = 0;
// Direction of the key msg scroll (0 - left to right, 1 - right to left)
unsigned char key_scrolling_direction = 0;


volatile char transactionDataKey[32];
volatile char transactionDataValue[32];
volatile char pageInfo[20];

int transactionDetailsCurrentPage;
int transactionDetailsPageCount;

void start_transaction_info_display(unsigned int unused);
void view_sign_transaction(unsigned int unused);
void reject(unsigned int unused);

//------ View elements
const ux_menu_entry_t menu_main[];
const ux_menu_entry_t menu_about[];

const ux_menu_entry_t menu_transaction_info[] = {
    {NULL, start_transaction_info_display, 0, NULL, "View transaction", NULL, 0, 0},
    {NULL, view_sign_transaction, 0, NULL, "Sign transaction", NULL, 0, 0},
    {NULL, reject, 0, &C_icon_back, "Reject", NULL, 60, 40},
    UX_MENU_END
};

const ux_menu_entry_t menu_main[] = {
    {NULL, NULL, 0, &C_icon_tendermint, "Tendermint", "Cosmos Demo", 33, 12},
    {menu_about, NULL, 0, NULL, "About", NULL, 0, 0},
    {NULL, os_sched_exit, 0, &C_icon_dashboard, "Quit app", NULL, 50, 29},
    UX_MENU_END
};

const ux_menu_entry_t menu_about[] = {
    {NULL, NULL, 0, NULL, "Version", APPVERSION, 0, 0},
    {menu_main, NULL, 2, &C_icon_back, "Back", NULL, 61, 40},
    UX_MENU_END
};

static const bagl_element_t bagl_ui_sign_transaction[] = {
    UI_FillRectangle(0, 0, 0, 128, 32, 0x000000, 0xFFFFFF),
    UI_Icon(0, 3, 32 / 2 - 4, 7, 7, BAGL_GLYPH_ICON_CROSS),
    UI_LabelLine(1, 0, 12, 128, 11, 0xFFFFFF, 0x000000, "Sign transaction"),
    UI_LabelLine(2, 0, 23, 128, 11, 0xFFFFFF, 0x000000, "Not implemented yet"),
};

static const bagl_element_t bagl_ui_transaction_info[] = {
    UI_FillRectangle(0, 0, 0, 128, 32, 0x000000, 0xFFFFFF),
    UI_Icon(0, 0, 0, 7, 7, BAGL_GLYPH_ICON_LEFT),
    UI_Icon(0, 128-7, 0, 7, 7, BAGL_GLYPH_ICON_RIGHT),
    UI_LabelLine(1, 0, 8, 128, 11, 0xFFFFFF, 0x000000,(const char*)pageInfo),
    UI_LabelLine(2, 0, 19, 128, 11, 0xFFFFFF, 0x000000,(const char*)transactionDataKey),
    UI_LabelLine(3, 0, 30, 128, 11, 0xFFFFFF, 0x000000,(const char*)transactionDataValue),
};
//------ View elements

//------ Event handlers
delegate_update_transaction_info event_handler_update_transaction_info = NULL;
delegate_reject_transaction event_handler_reject_transaction = NULL;
delegate_sign_transaction event_handler_sign_transaction = NULL;

void view_add_update_transaction_info_event_handler(delegate_update_transaction_info delegate)
{
    event_handler_update_transaction_info = delegate;
}

void view_add_reject_transaction_event_handler(delegate_reject_transaction delegate)
{
    event_handler_reject_transaction = delegate;
}

void view_add_sign_transaction_event_handler(delegate_sign_transaction delegate)
{
    event_handler_sign_transaction = delegate;
}
// ------ Event handlers


static unsigned int bagl_ui_sign_transaction_button(unsigned int button_mask,
                                                    unsigned int button_mask_counter)
{
    switch (button_mask) {
        default:
            view_display_transaction_menu(0);
    }
    return 0;
}

const bagl_element_t* ui_transaction_info_prepro(const bagl_element_t *element) {

    if (element->component.userid == 0) {
        update_transaction_page_info();

        if (key_scrolling_total_size > MAX_CHARS_PER_LINE) {
            key_scrolling_step_count = key_scrolling_total_size - MAX_CHARS_PER_LINE;
            UX_CALLBACK_SET_INTERVAL(500);
        } else {
            key_scrolling_step_count = 0;
        }

        if (view_scrolling_total_size > MAX_CHARS_PER_LINE) {
            view_scrolling_step_count = view_scrolling_total_size - MAX_CHARS_PER_LINE;
            if (view_scrolling_step == 0 || view_scrolling_step == view_scrolling_step_count-1)   {
                UX_CALLBACK_SET_INTERVAL(2000);
            }
            else {
                UX_CALLBACK_SET_INTERVAL(500);
            }
        } else {
            view_scrolling_step_count = 0;
        }
    }

    return element;
}

void reset_scrolling()
{
    view_scrolling_step = 0;
    view_scrolling_direction = 0;
    key_scrolling_step = 0;
    key_scrolling_direction = 0;
}

static unsigned int bagl_ui_transaction_info_button(unsigned int button_mask,
                                                    unsigned int button_mask_counter)
{
    switch (button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT:
            view_display_transaction_menu(0);
            break;

        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
            if (transactionDetailsCurrentPage > 0) {
                transactionDetailsCurrentPage--;
                reset_scrolling();
                UX_DISPLAY(bagl_ui_transaction_info, ui_transaction_info_prepro);
            } else {
                view_display_transaction_menu(0);
            }

            break;
        case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
            if (transactionDetailsCurrentPage < transactionDetailsPageCount - 1) {
                transactionDetailsCurrentPage++;
                reset_scrolling();
                UX_DISPLAY(bagl_ui_transaction_info, ui_transaction_info_prepro);
            } else {
                view_display_transaction_menu(0);
            }
            break;
    }
    return 0;
}

void start_transaction_info_display(unsigned int unused)
{
    UNUSED(unused);
    transactionDetailsCurrentPage = 0;
    reset_scrolling();
    UX_DISPLAY(bagl_ui_transaction_info, ui_transaction_info_prepro);
}

void update_transaction_page_info()
{
    if (event_handler_update_transaction_info != NULL) {
        event_handler_update_transaction_info(
                (char *) transactionDataKey,
                (char *) transactionDataValue,
                transactionDetailsCurrentPage);


        switch (current_sigtype)
        {
        case SECP256K1:
            snprintf(
                    (char *) pageInfo,
                    sizeof(pageInfo),
                    "SECP256K1 - %02d/%02d",
                    transactionDetailsCurrentPage + 1,
                    transactionDetailsPageCount);
            break;
        case ED25519:
            snprintf(
                    (char *) pageInfo,
                    sizeof(pageInfo),
                    "ED25519 - %02d/%02d",
                    transactionDetailsCurrentPage + 1,
                    transactionDetailsPageCount);
            break;
        }

    }
}

void view_sign_transaction(unsigned int unused)
{
    UNUSED(unused);

    if (event_handler_sign_transaction != NULL) {
        event_handler_sign_transaction();
    }
    else {
        UX_DISPLAY(bagl_ui_sign_transaction, NULL);
    }
}

void reject(unsigned int unused)
{
    if (event_handler_reject_transaction != NULL) {
        event_handler_reject_transaction();
    }
}

void io_seproxyhal_display(const bagl_element_t *element)
{
    io_seproxyhal_display_default((bagl_element_t *) element);
}

void view_init(void)
{
    UX_INIT();
    view_uiState = UI_IDLE;
}

void view_idle(unsigned int ignored)
{
    view_uiState = UI_IDLE;
    UX_MENU_DISPLAY(0, menu_main, NULL);
}

void view_display_transaction_menu(unsigned int numberOfTransactionPages)
{
    if (numberOfTransactionPages != 0) {
        transactionDetailsPageCount = numberOfTransactionPages;
    }
    view_uiState = UI_TRANSACTION;
    UX_MENU_DISPLAY(0, menu_transaction_info, NULL);
}

void view_display_signing_success()
{
    // TODO Add view
    view_idle(0);
}

void view_display_signing_error()
{
    // TODO Add view
    view_idle(0);
}