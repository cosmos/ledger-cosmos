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

#include "view.h"
#include "view_templates.h"
#include "view_expl.h"
#include "common.h"

#include "glyphs.h"
#include "bagl.h"
#include "zxmacros.h"

#include <string.h>
#include <stdio.h>

void viewctl_display_page();

enum UI_DISPLAY_MODE viewctl_scrolling_mode;

volatile char viewctl_DataKey[MAX_CHARS_PER_KEY_LINE];
volatile char viewctl_DataValue[MAX_CHARS_PER_VALUE_LINE];
volatile char viewctl_Title[MAX_SCREEN_LINE_WIDTH];
const char *dblClickInfo = "DBL-CLICK TO VIEW";

int viewctl_DetailsCurrentPage;
int viewctl_DetailsPageCount;
int viewctl_ChunksIndex;
int viewctl_ChunksCount;

viewctl_delegate_getData viewctl_ehGetData = NULL;
viewctl_delegate_ready viewctl_ehReady = NULL;
viewctl_delegate_exit viewctl_ehExit = NULL;
viewctl_delegate_display_ux viewctl_display_ux = NULL;

//------ External functions

void viewctl_start(int start_page,
                   viewctl_delegate_getData func_getData,
                   viewctl_delegate_ready ehReady,
                   viewctl_delegate_exit ehExit,
                   viewctl_delegate_display_ux func_display_ux) {

    viewctl_ehGetData = func_getData;
    viewctl_ehReady = ehReady;
    viewctl_ehExit = ehExit;
    viewctl_display_ux = func_display_ux;

    viewctl_scrolling_mode = PENDING;
    viewctl_DetailsCurrentPage = start_page;

    viewctl_ChunksIndex = 0;
    viewctl_ChunksCount = 1;

    viewctl_display_page();

    // TODO: Should this happen here?
    if (viewctl_ehReady != NULL) {
        ehReady(0);
    }
}

//------ Event handlers

const bagl_element_t *ui_view_info_prepro(const bagl_element_t *element) {

    switch (element->component.userid) {
        case 0x01:
            UX_CALLBACK_SET_INTERVAL(2000);
            break;
        case 0x02:
            UX_CALLBACK_SET_INTERVAL(MAX(3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
            break;
        case 0x03:
            UX_CALLBACK_SET_INTERVAL(MAX(3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
            break;
    }
    return element;
}

void submenu_left() {
    viewctl_ChunksIndex--;
    viewctl_scrolling_mode = PENDING;
    viewctl_display_page();
}

void submenu_right() {
    viewctl_ChunksIndex++;
    viewctl_scrolling_mode = PENDING;
    viewctl_display_page();
}

void menu_left() {
    viewctl_scrolling_mode = PENDING;
    viewctl_ChunksIndex = 0;
    viewctl_ChunksCount = 1;
    if (viewctl_DetailsCurrentPage > 0) {
        viewctl_DetailsCurrentPage--;
        viewctl_display_page();
    } else {
        viewctl_ehExit(0);
    }
}

void menu_right() {
    viewctl_scrolling_mode = PENDING;
    viewctl_ChunksIndex = 0;
    viewctl_ChunksCount = 1;
    if (viewctl_DetailsCurrentPage < viewctl_DetailsPageCount - 1) {
        viewctl_DetailsCurrentPage++;
        viewctl_display_page();
    } else {
        viewctl_ehExit(0);
    }
}

void viewctl_crop_key() {
    int offset = strlen((char *) viewctl_DataKey) - MAX_SCREEN_LINE_WIDTH;
    if (offset > 0) {
        char *start = (char *) viewctl_DataKey;
        for (;;) {
            *start = start[offset];
            if (*start++ == '\0')
                break;
        }
    }
}

void viewctl_display_page() {
    if (viewctl_ehGetData == NULL) {
        return;
    }

    // Read key and value strings from json
    viewctl_ehGetData(
            (char *) viewctl_Title,
            sizeof(viewctl_Title),
            (char *) viewctl_DataKey,
            sizeof(viewctl_DataKey),
            (char *) viewctl_DataValue,
            sizeof(viewctl_DataValue),
            viewctl_DetailsCurrentPage,
            viewctl_ChunksIndex,
            &viewctl_DetailsPageCount,
            &viewctl_ChunksCount);

    // If value is very long, we split it into chunks
    // and add chunk index/count information at the end of the key
    if (viewctl_ChunksCount > 1) {
        int position = strlen((char *) viewctl_DataKey);
        snprintf((char *) viewctl_DataKey + position,
                 sizeof(viewctl_DataKey) - position,
                 " %02d/%02d",
                 viewctl_ChunksIndex + 1,
                 viewctl_ChunksCount);
    }

    switch (viewctl_scrolling_mode) {
        case KEY_SCROLLING_NO_VALUE: {
            viewctl_crop_key();
            viewctl_scrolling_mode = VALUE_SCROLLING;
            break;
        }
        case PENDING: {
            viewctl_scrolling_mode = VALUE_SCROLLING;
            if (strlen((char *) viewctl_DataKey) > MAX_SCREEN_LINE_WIDTH) {
                int value_length = strlen((char *) viewctl_DataValue);
                if (value_length > MAX_SCREEN_LINE_WIDTH) {
                    strcpy((char *) viewctl_DataValue, "DBL-CLICK FOR VALUE");
                    viewctl_scrolling_mode = KEY_SCROLLING_NO_VALUE;
                } else {
                    viewctl_scrolling_mode = KEY_SCROLLING_SHORT_VALUE;
                }
            }
        }
        default:
            break;
    }

    // fix possible utf8 issues
    asciify((char *) viewctl_Title);
    asciify((char *) viewctl_DataKey);
    asciify((char *) viewctl_DataValue);

    viewctl_display_ux();
}
