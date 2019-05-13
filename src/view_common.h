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

#include <stdint.h>

#include "os.h"
#include "cx.h"

#define MAX_CHARS_TITLE             32
#define MAX_CHARS_PER_KEY_LINE      64
#define MAX_CHARS_PER_VALUE_LINE    192
#define MAX_SCREEN_LINE_WIDTH       19

enum UI_DISPLAY_MODE {
    VALUE_SCROLLING,
    KEY_SCROLLING_NO_VALUE,
    KEY_SCROLLING_SHORT_VALUE,
    PENDING
};

#if defined(TARGET_NANOX)
#define MAX_SCREEN_NUM_LINES       4
#endif

typedef struct {
    enum UI_DISPLAY_MODE scrolling_mode;
    // Index of the currently displayed page
    int16_t detailsCurrentPage;
    // Total number of displayable pages
    int16_t detailsPageCount;

    // When data goes beyond the limit, it will be split in chunks that
    // that spread over several pages
    // Index of currently displayed value chunk
    int16_t chunksIndex;
    // Total number of displayable value chunks
    int16_t chunksCount;

    // DATA
    char title[MAX_SCREEN_LINE_WIDTH];
    char dataKey[MAX_CHARS_PER_KEY_LINE];
    char dataValue[MAX_CHARS_PER_VALUE_LINE];

#if defined(TARGET_NANOX)
    char dataValueChunk[MAX_SCREEN_NUM_LINES][MAX_SCREEN_LINE_WIDTH+1];
#endif
} viewctl_s;

extern viewctl_s viewctl;

// Delegate to update contents
typedef int16_t (*viewctl_delegate_getData)(
        char *title, int16_t max_title_length,
        char *key, int16_t max_key_length,
        char *value, int16_t max_value_length,
        int16_t page_index,
        int16_t chunk_index,
        int16_t *page_count_out,
        int16_t *chunk_count_out);

// Delegate to handle exit view event
typedef void (*viewctl_delegate_exit)(unsigned int ignored);

// Delegate to handle exit view event
typedef void (*viewctl_delegate_ready)(unsigned int ignored);

// Delegate to handle exit view event
typedef void (*viewctl_delegate_display_ux)();

// Delegate to handle an accept event
typedef void (*viewctl_delegate_accept)();

// Delegate to handle a reject event
typedef void (*viewctl_delegate_reject)();

extern viewctl_delegate_getData viewctl_ehGetData;
extern viewctl_delegate_ready viewctl_ehReady;
extern viewctl_delegate_exit viewctl_ehExit;
extern viewctl_delegate_display_ux viewctl_display_ux;

void viewctl_start(int start_page,
                   viewctl_delegate_getData ehUpdate,
                   viewctl_delegate_ready ehReady,
                   viewctl_delegate_exit ehExit,
                   viewctl_delegate_display_ux func_display_ux);

void viewctl_display_page();

void submenu_left();

void submenu_right();

void menu_left();

void menu_right();

#define print_title(...) snprintf(viewctl.title, sizeof(viewctl.title), __VA_ARGS__)

#define print_key(...) snprintf(viewctl.dataKey, sizeof(viewctl.dataKey), __VA_ARGS__)

#define print_value(...) snprintf(viewctl.dataValue, sizeof(viewctl.dataValue), __VA_ARGS__)

void viewctl_dataValue_split();
