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
#pragma once

#include "os.h"
#include "cx.h"
#include <os_io_seproxyhal.h>

#define UI_CENTER11PX       BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER
#define UI_CENTER11PX_BOLD  BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER
#define DEFAULT_FONT        BAGL_FONT_OPEN_SANS_LIGHT_16px | BAGL_FONT_ALIGNMENT_LEFT

enum UI_STATE {
    UI_IDLE,
    UI_WAITING_FOR_DATA,
    UI_TRANSACTION
};

extern enum UI_STATE uiState;

void ui_update_transaction_info(const char* inputAddress,
                                int addressSize,
                                const char* inputCoinName,
                                int coinNameSize,
                                const char* inputCoinAmount,
                                int coinAmountSize);
void ui_init(void);
void ui_idle(void);
void ui_wait_for_data(unsigned int ignored);
void ui_display_transaction(unsigned int ignored);
void menu_transaction_display_input_address(unsigned int ignored);
void menu_transaction_display_input_coin_name(unsigned int ignored);
void menu_transaction_display_input_coin_amount(unsigned int ignored);
