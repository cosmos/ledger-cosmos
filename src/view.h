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
#include "view_common.h"
#include "view_expl.h"
#include "view_conf.h"

//------ Event handlers
/// view_set_handlers
void view_set_handlers(viewctl_delegate_getData func_getData,
                       viewctl_delegate_accept func_accept,
                       viewctl_delegate_reject func_reject);

//------ Common functions
/// view_init
void view_init(void);

/// view_idle
void view_idle(unsigned int ignored);

/// view_tx_menu
void view_tx_menu(unsigned int ignored);

/// view_tx_show
void view_tx_show(unsigned int start_page);

/// view_addr_confirm
void view_addr_confirm(unsigned int start_page);

/// view_addr_show
void view_addr_show(unsigned int start_page);

int view_tx_get_data(char *title, int max_title_length,
                     char *key, int max_key_length,
                     char *value, int max_value_length,
                     int page_index,
                     int chunk_index,
                     int *page_count_out,
                     int *chunk_count_out);
