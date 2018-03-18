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
#include <string.h>
#include "ui.h"
#include "glyphs.h"

ux_state_t ux;
enum UI_STATE uiState;
volatile char inputAddress[32];
volatile char inputCoinName[10];
volatile char inputCoinAmount[10];

const ux_menu_entry_t menu_main[];
//const ux_menu_entry_t menu_settings[];
//const ux_menu_entry_t menu_settings_data[];

// change the setting
//void menu_settings_data_change(unsigned int enabled) {
//    // go back to the menu entry
//    UX_MENU_DISPLAY(0, menu_settings, NULL);
//}
//
//// show the currently activated entry
//void menu_settings_data_init(unsigned int ignored) {
//    UNUSED(ignored);
//    UX_MENU_DISPLAY(1, menu_settings_data, NULL);
//}

// {{type, userid, x, y, width, height, stroke, radius, fill, fgcolor, bgcolor, font_id, icon_id},
//   text, touch_area_brim, overfgcolor,  overbgcolor, tap, out, over, },
static const bagl_element_t bagl_ui_waiting[] =
{
    {
            {BAGL_RECTANGLE, 0x00, 0, 0, 128, 32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF, 0, 0},
            NULL, 0, 0, 0, NULL, NULL, NULL,
    },
    {
            {BAGL_LABELINE, 0x02, 0, 12, 128, 11, 0, 0, 0, 0xFFFFFF, 0x000000, UI_CENTER11PX, 0},
            "Waiting for", 0, 0, 0, NULL, NULL, NULL,
    },
    {
            {BAGL_LABELINE, 0x02, 0, 23, 128, 11, 0, 0, 0, 0xFFFFFF, 0x000000, UI_CENTER11PX, 0},
            "data...", 0, 0, 0, NULL, NULL, NULL,
    },
    {
            {BAGL_ICON, 0x00, 3, 12, 7,   7,  0, 0, 0, 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CROSS},
            NULL, 0, 0, 0, NULL, NULL, NULL,
    },
};

// {{type, userid, x, y, width, height, stroke, radius, fill, fgcolor, bgcolor, font_id, icon_id},
//   text, touch_area_brim, overfgcolor,  overbgcolor, tap, out, over, },
//static const bagl_element_t bagl_ui_idle_nanos[] =
//{
//    {
//        {BAGL_RECTANGLE, 0x00, 0, 0, 128, 32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF, 0, 0},
//        NULL, 0, 0, 0, NULL, NULL, NULL,
//    },
//    {
//        {BAGL_LABELINE, 0x02, 0, 12, 128, 11, 0, 0, 0, 0xFFFFFF, 0x000000, UI_CENTER11PX, 0},
//        "Tendermint", 0, 0, 0, NULL, NULL, NULL,
//    },
//    {
//        {BAGL_LABELINE, 0x02, 0, 23, 128, 11, 0, 0, 0, 0xFFFFFF, 0x000000, UI_CENTER11PX, 0},
//        "Cosmos Demo", 0, 0, 0, NULL, NULL, NULL,
//    },
//    {
//        {BAGL_ICON, 0x00, 3, 12, 7,   7,  0, 0, 0, 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CROSS},
//        NULL, 0, 0, 0, NULL, NULL, NULL,
//    },
//};


static unsigned int bagl_ui_waiting_button(unsigned int button_mask,
                                           unsigned int button_mask_counter) {
    switch (button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
        case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT:
            UX_MENU_DISPLAY(0, menu_main, NULL);
            break;
    }
    return 0;
}

// {{type, userid, x, y, width, height, stroke, radius, fill, fgcolor, bgcolor, font_id, icon_id},
//   text, touch_area_brim, overfgcolor,  overbgcolor, tap, out, over, },
static const bagl_element_t bagl_ui_input_address[] =
{
        {
                {BAGL_RECTANGLE, 0x00, 0, 0, 128, 32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF, 0, 0},
                NULL, 0, 0, 0, NULL, NULL, NULL,
        },
        {
                {BAGL_LABELINE, 0x02, 0, 12, 128, 11, 0, 0, 0, 0xFFFFFF, 0x000000, UI_CENTER11PX, 0},
                "Input address", 0, 0, 0, NULL, NULL, NULL,
        },
        {
                {BAGL_LABELINE, 0x02, 0, 23, 128, 11, 0, 0, 0, 0xFFFFFF, 0x000000, UI_CENTER11PX, 0},
                (const char*)inputAddress, 0, 0, 0, NULL, NULL, NULL,
        },
        {
                {BAGL_ICON, 0x00, 3, 12, 7,   7,  0, 0, 0, 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CROSS},
                NULL, 0, 0, 0, NULL, NULL, NULL,
        },
};

static unsigned int bagl_ui_input_address_button(unsigned int button_mask,
                                                 unsigned int button_mask_counter) {
    switch (button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
        case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT:
            ui_display_transaction(0);
            break;
    }
    return 0;
}

// {{type, userid, x, y, width, height, stroke, radius, fill, fgcolor, bgcolor, font_id, icon_id},
//   text, touch_area_brim, overfgcolor,  overbgcolor, tap, out, over, },
static const bagl_element_t bagl_ui_input_coin_name[] =
{
    {
            {BAGL_RECTANGLE, 0x00, 0, 0, 128, 32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF, 0, 0},
            NULL, 0, 0, 0, NULL, NULL, NULL,
    },
    {
            {BAGL_LABELINE, 0x02, 0, 12, 128, 11, 0, 0, 0, 0xFFFFFF, 0x000000, UI_CENTER11PX, 0},
            "Input coin name", 0, 0, 0, NULL, NULL, NULL,
    },
    {
            {BAGL_LABELINE, 0x02, 0, 23, 128, 11, 0, 0, 0, 0xFFFFFF, 0x000000, UI_CENTER11PX, 0},
            (const char*)inputCoinName, 0, 0, 0, NULL, NULL, NULL,
    },
    {
            {BAGL_ICON, 0x00, 3, 12, 7,   7,  0, 0, 0, 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CROSS},
            NULL, 0, 0, 0, NULL, NULL, NULL,
    },
};

static unsigned int bagl_ui_input_coin_name_button(unsigned int button_mask,
                                                   unsigned int button_mask_counter) {
    switch (button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
        case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT:
            ui_display_transaction(0);
            break;
    }
    return 0;
}

// {{type, userid, x, y, width, height, stroke, radius, fill, fgcolor, bgcolor, font_id, icon_id},
//   text, touch_area_brim, overfgcolor,  overbgcolor, tap, out, over, },
static const bagl_element_t bagl_ui_input_coin_amount[] =
{
    {
            {BAGL_RECTANGLE, 0x00, 0, 0, 128, 32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF, 0, 0},
            NULL, 0, 0, 0, NULL, NULL, NULL,
    },
    {
            {BAGL_LABELINE, 0x02, 0, 12, 128, 11, 0, 0, 0, 0xFFFFFF, 0x000000, UI_CENTER11PX, 0},
            "Input coin amount", 0, 0, 0, NULL, NULL, NULL,
    },
    {
            {BAGL_LABELINE, 0x02, 0, 23, 128, 11, 0, 0, 0, 0xFFFFFF, 0x000000, UI_CENTER11PX, 0},
            (const char*)inputCoinAmount, 0, 0, 0, NULL, NULL, NULL,
    },
    {
            {BAGL_ICON, 0x00, 3, 12, 7,   7,  0, 0, 0, 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CROSS},
            NULL, 0, 0, 0, NULL, NULL, NULL,
    },
};

static unsigned int bagl_ui_input_coin_amount_button(unsigned int button_mask,
                                                     unsigned int button_mask_counter) {
    switch (button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
        case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT:
            ui_display_transaction(0);
            break;
    }
    return 0;
}

void ui_update_transaction_info(const char* address,
                                int addressSize,
                                const char* coinName,
                                int coinNameSize,
                                const char* coinAmount,
                                int coinAmountSize)
{
    os_memmove((char*)inputAddress, address, addressSize);
    inputAddress[addressSize+1] = '\0';
    os_memmove((char*)inputCoinName, coinName, coinNameSize);
    inputCoinName[coinNameSize+1] = '\0';
    os_memmove((char*)inputCoinAmount, coinAmount, coinAmountSize);
    inputCoinAmount[coinAmountSize+1] = '\0';
}

const ux_menu_entry_t menu_transaction_info[] = {
        {NULL, menu_transaction_display_input_address, 0, NULL, "Input address", NULL, 0, 0},
        {NULL, menu_transaction_display_input_coin_name, 0, NULL, "Input coin name", NULL, 0, 0},
        {NULL, menu_transaction_display_input_coin_amount, 0, NULL, "Input coin amount", NULL, 0, 0},
        {NULL, ui_wait_for_data, 1, &C_icon_back, "Back", NULL, 61, 40},
        UX_MENU_END
};

//
//const ux_menu_entry_t menu_settings_data[] = {
//        {NULL, menu_settings_data_change, 0, NULL, "No", NULL, 0, 0},
//        {NULL, menu_settings_data_change, 0, NULL, "No", NULL, 0, 0},
//        {NULL, menu_settings_data_change, 1, NULL, "Yes", NULL, 0, 0},
//        UX_MENU_END};

const ux_menu_entry_t menu_about[] = {
        {NULL, NULL, 0, NULL, "Version", APPVERSION, 0, 0},
        {menu_main, NULL, 2, &C_icon_back, "Back", NULL, 61, 40},
        UX_MENU_END
};

const ux_menu_entry_t menu_main[] = {
        {NULL, NULL, 0, &C_icon_tendermint, "Tendermint", "Cosmos Demo", 33, 12},
        {NULL, ui_wait_for_data, 0, NULL, "Wait for data", NULL, 0, 0},
        {menu_about, NULL, 0, NULL, "About", NULL, 0, 0},
        {NULL, os_sched_exit, 0, &C_icon_dashboard, "Quit app", NULL, 50, 29},
        UX_MENU_END
};

void io_seproxyhal_display(const bagl_element_t *element) {
    io_seproxyhal_display_default((bagl_element_t *) element);
}

//static const bagl_element_t *io_seproxyhal_touch_exit(const bagl_element_t *e) {
//    os_sched_exit(0);   // Go back to the dashboard
//    return NULL; // do not redraw the widget
//}

//static unsigned int bagl_ui_idle_nanos_button(unsigned int button_mask,
//                                              unsigned int button_mask_counter) {
//    switch (button_mask) {
//        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
//        case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT:
//            io_seproxyhal_touch_exit(NULL);
//            break;
//    }
//    return 0;
//}

void ui_init(void)
{
    UX_INIT();
    uiState = UI_IDLE;
}

void ui_idle(void)
{
    uiState = UI_IDLE;
    UX_MENU_DISPLAY(0, menu_main, NULL);
}

void ui_wait_for_data(unsigned int ignored)
{
    UNUSED(ignored);
    uiState = UI_WAITING_FOR_DATA;
    UX_DISPLAY(bagl_ui_waiting, NULL);
}

void ui_display_transaction(unsigned int ignored)
{
    UNUSED(ignored);
    uiState = UI_TRANSACTION;
    UX_MENU_DISPLAY(0, menu_transaction_info, NULL);
}

void menu_transaction_display_input_address(unsigned int ignored)
{
    UNUSED(ignored);
    UX_DISPLAY(bagl_ui_input_address, NULL);
}

void menu_transaction_display_input_coin_name(unsigned int ignored)
{
    UNUSED(ignored);
    UX_DISPLAY(bagl_ui_input_coin_name, NULL);
}

void menu_transaction_display_input_coin_amount(unsigned int ignored)
{
    UNUSED(ignored);
    UX_DISPLAY(bagl_ui_input_coin_amount, NULL);
}
