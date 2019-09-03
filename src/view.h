/*******************************************************************************
*   (c) 2018,2019 ZondaX GmbH
*   (c) 2016 Ledger
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

#include <stdint.h>

#if defined(LEDGER_SPECIFIC)
#include "bolos_target.h"
#if defined(BOLOS_SDK)
#include "os.h"
#include "cx.h"
#endif
#endif

#include "view_common.h"
#include "view_expl.h"

//------ Event handlers
/// view_set_handlers
void view_set_handlers(viewctl_delegate_getData func_getData,
                       viewctl_delegate_accept func_accept,
                       viewctl_delegate_reject func_reject);

//------ Common functions
/// view_init (initializes UI)
void view_init(void);

/// view_idle_show (idle view - main menu + status)
void view_idle_show(unsigned int ignored);

/// view_status
void view_status();

/// view_tx_show (show/review transaction view)
void view_sign_show();

/// view_addr_confirm (show/accept public key + address request)
void view_addr_confirm(unsigned int _);

void view_address_show();
