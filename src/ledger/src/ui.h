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
#define MAX_CHARS_PER_LINE 10

enum UI_STATE {
    UI_IDLE,
    UI_TRANSACTION
};

extern unsigned int ux_step;
extern unsigned int ux_step_count;
extern unsigned int ux_total_size;
extern unsigned int ux_direction;

extern enum UI_STATE uiState;

// Callback for updating transaction UI
typedef int (*UpdateTxDataPtr)(char*,int,char*,int,int);
void set_update_transaction_ui_data_callback(UpdateTxDataPtr ptr);

// Callback for rejecting current transaction
typedef void (*RejectPtr)();
void set_reject_transaction_callback(RejectPtr ptr);

void ui_init(void);
void ui_idle(unsigned int ignored);
void ui_wait_for_data(unsigned int ignored);
void display_transaction_menu(unsigned int ignored);
void update_transaction_page_info();
