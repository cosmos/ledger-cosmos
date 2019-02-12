/*******************************************************************************
*   (c) 2016 Ledger
*   (c) 2018, 2019 ZondaX GmbH
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

#define MAX_CHARS_TITLE             32
#define MAX_CHARS_PER_KEY_LINE      64
#define MAX_CHARS_PER_VALUE_LINE    192
#define MAX_SCREEN_LINE_WIDTH       22

enum UI_DISPLAY_MODE {
    VALUE_SCROLLING,
    KEY_SCROLLING_NO_VALUE,
    KEY_SCROLLING_SHORT_VALUE,
    PENDING
};

// Delegate to update contents
typedef int (*viewctl_delegate_update)(
        char *title, int max_title_length,
        char *key, int max_key_length,
        char *value, int max_value_length,
        int page_index,
        int chunk_index,
        int *page_count_out,
        int *chunk_count_out);

// Delegate to handle exit view event
typedef void (*viewctl_delegate_exit)(unsigned int ignored);

// Delegate to handle exit view event
typedef void (*viewctl_delegate_ready)(unsigned int ignored);

// Initialize and show control
void viewctl_start(
        viewctl_delegate_update delegate_update,
        viewctl_delegate_ready delegate_ready,
        viewctl_delegate_exit delegate_exit,
        int start_page);
