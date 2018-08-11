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

#define MAX_CHARS_PER_KEY_LINE      64
#define MAX_CHARS_PER_VALUE_LINE    128
#define MAX_SCREEN_LINE_WIDTH       22

enum UI_STATE {
    UI_IDLE,
    UI_TRANSACTION
};

extern enum UI_STATE view_uiState;

//------ Delegates definitions
typedef int (*delegate_update_transaction_info)(char*,int, char*, int, int, int*);
typedef void (*delegate_reject_transaction)();

typedef void (*delegate_sign_transaction)();

//------ Event handlers
/// view_add_update_transaction_info_event_handler
/// \param delegate
void view_add_update_transaction_info_event_handler(delegate_update_transaction_info delegate);

/// view_add_reject_transaction_event_handler
/// \param delegate
void view_add_reject_transaction_event_handler(delegate_reject_transaction delegate);

/// view_add_sign_transaction_event_handler
/// \param delegate
void view_add_sign_transaction_event_handler(delegate_sign_transaction delegate);

//------ Common functions
/// view_init
void view_init(void);

/// view_idle
void view_idle(unsigned int ignored);

/// view_display_transaction_menu
void view_display_transaction_menu(unsigned int ignored);

/// view_display_signing_success
void view_display_signing_success();

/// view_display_signing_error
void view_display_signing_error();
