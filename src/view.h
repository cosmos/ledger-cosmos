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
#include "view_ctl.h"

enum UI_STATE {
    UI_IDLE,
    UI_TRANSACTION
};

extern enum UI_STATE view_uiState;

//------ Delegates definitions
typedef void (*delegate_reject_tx)();

typedef void (*delegate_sign_tx)();

//------ Event handlers
/// view_set_sign_tx_event_handler
/// \param delegate
void view_set_tx_event_handlers(viewctl_delegate_update ehUpdate,
                                delegate_sign_tx ehSign,
                                delegate_reject_tx ehReject);

/// view_set_addr_event_handlers
/// \param delegate
void view_set_addr_event_handlers(viewctl_delegate_update ehUpdate);

//------ Common functions
/// view_init
void view_init(void);

/// view_idle
void view_idle(unsigned int ignored);

/// view_display_tx_menu
void view_display_tx_menu(unsigned int ignored);

/// view_tx_show
void view_tx_show(unsigned int unused);

/// view_addr_show
void view_addr_show(unsigned int unused);

/// view_display_signing_success
void view_display_signing_success();

/// view_display_signing_error
void view_display_signing_error();
