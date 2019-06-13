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

#include "glyphs.h"
#include "bagl.h"
#include "zxmacros.h"

#include <string.h>
#include <stdio.h>

#if defined(TARGET_NANOX)
static const bagl_element_t viewexpl_bagl[] = {
    UI_BACKGROUND_LEFT_RIGHT_ICONS,
    UI_LabelLine(UIID_LABEL+0, 0, 9 + UI_11PX * 0, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, (const char *) viewctl.title),
    UI_LabelLine(UIID_LABEL+1, 0, 9 + UI_11PX * 1, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, (const char *) viewctl.dataKey),
    UI_LabelLine(UIID_LABEL+2, 0, 9 + UI_11PX * 2, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, (const char *) viewctl.dataValueChunk[0]),
    UI_LabelLine(UIID_LABEL+3, 0, 9 + UI_11PX * 3, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, (const char *) viewctl.dataValueChunk[1]),
    UI_LabelLine(UIID_LABEL+4, 0, 9 + UI_11PX * 4, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, (const char *) viewctl.dataValueChunk[2]),
    UI_LabelLine(UIID_LABEL+5, 0, 9 + UI_11PX * 5, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, (const char *) viewctl.dataValueChunk[3]),
};

static unsigned int viewexpl_bagl_button(
    unsigned int button_mask,
    unsigned int button_mask_counter) {
    switch (button_mask) {
        // Press both left and right to switch to value scrolling
        case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT: {
            if (viewctl.scrolling_mode == KEY_SCROLLING_NO_VALUE) {
                viewctl_display_page();
            } else {
                viewctl_ehExit(0);
            }
            break;
        }

            // Press left to progress to the previous element
        case BUTTON_EVT_RELEASED | BUTTON_LEFT: {
            if (viewctl.chunksIndex > 0) {
                submenu_left();
            } else {
                menu_left();
            }
            break;
        }

            // Hold left to progress to the previous element quickly
            // It also steps out from the value chunk page view mode
        case BUTTON_EVT_FAST | BUTTON_LEFT: {
            menu_left();
            break;
        }

            // Press right to progress to the next element
        case BUTTON_EVT_RELEASED | BUTTON_RIGHT: {
            if (viewctl.chunksIndex < viewctl.chunksCount - 1) {
                submenu_right();
            } else {
                menu_right();
            }
            break;
        }

            // Hold right to progress to the next element quickly
            // It also steps out from the value chunk page view mode
        case BUTTON_EVT_FAST | BUTTON_RIGHT: {
            menu_right();
            break;
        }
    }
    return 0;
}

#elif  defined(TARGET_NANOS)
static const bagl_element_t viewexpl_bagl_valuescrolling[] = {
    UI_BACKGROUND_LEFT_RIGHT_ICONS,
    UI_LabelLine(UIID_LABEL + 0, 0, 8, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, (const char *) viewctl.title),
    UI_LabelLine(UIID_LABEL + 1, 0, 19, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, (const char *) viewctl.dataKey),
    UI_LabelLineScrolling(UIID_LABELSCROLL, 16, 30, 96, UI_11PX, UI_WHITE, UI_BLACK, (const char *) viewctl.dataValue),
};

static const bagl_element_t viewexpl_bagl_keyscrolling[] = {
    UI_BACKGROUND_LEFT_RIGHT_ICONS,
    UI_LabelLine(UIID_LABEL + 0, 0, 8, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, (const char *) viewctl.title),
    UI_LabelLine(UIID_LABEL + 1, 0, 30, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, (const char *) viewctl.dataValue),
    UI_LabelLineScrolling(UIID_LABELSCROLL, 16, 19, 96, UI_11PX, UI_WHITE, UI_BLACK, (const char *) viewctl.dataKey),
};

static unsigned int viewexpl_bagl_keyscrolling_button(
        unsigned int button_mask,
        unsigned int button_mask_counter) {
    switch (button_mask) {
        // Press both left and right to switch to value scrolling
        case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT: {
            if (viewctl.scrolling_mode == KEY_SCROLLING_NO_VALUE) {
                viewctl_display_page();
            } else {
                viewctl_ehExit(0);
            }
            break;
        }

            // Press left to progress to the previous element
        case BUTTON_EVT_RELEASED | BUTTON_LEFT: {
            if (viewctl.chunksIndex > 0) {
                submenu_left();
            } else {
                menu_left();
            }
            break;
        }

            // Hold left to progress to the previous element quickly
            // It also steps out from the value chunk page view mode
        case BUTTON_EVT_FAST | BUTTON_LEFT: {
            menu_left();
            break;
        }

            // Press right to progress to the next element
        case BUTTON_EVT_RELEASED | BUTTON_RIGHT: {
            if (viewctl.chunksIndex < viewctl.chunksCount - 1) {
                submenu_right();
            } else {
                menu_right();
            }
            break;
        }

            // Hold right to progress to the next element quickly
            // It also steps out from the value chunk page view mode
        case BUTTON_EVT_FAST | BUTTON_RIGHT: {
            menu_right();
            break;
        }
    }
    return 0;
}

static unsigned int viewexpl_bagl_valuescrolling_button(
        unsigned int button_mask,
        unsigned int button_mask_counter) {
    return viewexpl_bagl_keyscrolling_button(button_mask, button_mask_counter);
}

#endif

const bagl_element_t *viewexpl_bagl_prepro(const bagl_element_t *element) {
    switch (element->component.userid) {
        case UIID_ICONLEFT:
            UX_CALLBACK_SET_INTERVAL(2000);
            break;
        case UIID_ICONRIGHT:
            UX_CALLBACK_SET_INTERVAL(2000);
            break;
        case UIID_LABELSCROLL:
            UX_CALLBACK_SET_INTERVAL(MAX(3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
            break;
    }
    return element;
}

void viewexpl_display_ux() {
#if defined(TARGET_NANOX)
    UX_DISPLAY(viewexpl_bagl, viewexpl_bagl_prepro);
#else
    if (viewctl.scrolling_mode == VALUE_SCROLLING) {
        UX_DISPLAY(viewexpl_bagl_valuescrolling, viewexpl_bagl_prepro);
    } else {
        UX_DISPLAY(viewexpl_bagl_keyscrolling, viewexpl_bagl_prepro);
    }
#endif
}

void viewexpl_start(int start_page,
                    viewctl_delegate_getData ehUpdate,
                    viewctl_delegate_ready ehReady,
                    viewctl_delegate_exit ehExit) {
    viewctl_start(start_page, ehUpdate, ehReady, ehExit, viewexpl_display_ux);
}
