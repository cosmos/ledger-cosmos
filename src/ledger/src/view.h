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

#define UI_CENTER11PX       BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER
#define UI_CENTER11PX_BOLD  BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER
#define DEFAULT_FONT        BAGL_FONT_OPEN_SANS_LIGHT_16px | BAGL_FONT_ALIGNMENT_LEFT
#define MAX_CHARS_PER_LINE 20

enum UI_STATE {
    UI_IDLE,
    UI_TRANSACTION
};

//------ Public data (TODO review)
extern unsigned int view_scrolling_step;
extern unsigned int view_scrolling_step_count;
extern unsigned int view_scrolling_total_size;
extern unsigned int view_scrolling_direction;
extern enum UI_STATE view_uiState;

//------ Delegates definitions
typedef int (*delegate_update_transaction_info)(char*,char*,int);
typedef void (*delegate_reject_transaction)();
typedef void (*delegate_sign_transaction)();

//------ Event handlers
void view_add_update_transaction_info_event_handler(delegate_update_transaction_info delegate);
void view_add_reject_transaction_event_handler(delegate_reject_transaction delegate);
void view_add_sign_transaction_event_handler(delegate_sign_transaction delegate);

//------ Common functions (TODO review)
void view_init(void);
void view_idle(unsigned int ignored);
void view_display_transaction_menu(unsigned int ignored);
void view_display_signing_success();
void view_display_signing_error();

