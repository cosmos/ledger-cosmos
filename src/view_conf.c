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

viewctl_delegate_accept viewctl_ehAccept = NULL;
viewctl_delegate_reject viewctl_ehReject = NULL;

static const bagl_element_t viewconf_bagl_valuescrolling[] = {
        UI_FillRectangle(0, 0, 0, 128, 32, 0x000000, 0xFFFFFF),
        UI_Icon(0, 0, 0, 7, 7, BAGL_GLYPH_ICON_CROSS),
        UI_Icon(0, 128 - 7, 0, 7, 7, BAGL_GLYPH_ICON_CHECK),
        UI_LabelLine(1, 0, 8, 128, 11, 0xFFFFFF, 0x000000, (const char *) viewctl_Title),
        UI_LabelLine(1, 0, 19, 128, 11, 0xFFFFFF, 0x000000, (const char *) viewctl_DataKey),
        UI_LabelLineScrolling(2, 16, 30, 96, 11, 0xFFFFFF, 0x000000, (const char *) viewctl_DataValue),
};

static const bagl_element_t viewconf_bagl_keyscrolling[] = {
        UI_FillRectangle(0, 0, 0, 128, 32, 0x000000, 0xFFFFFF),
        UI_Icon(0, 0, 0, 7, 7, BAGL_GLYPH_ICON_CROSS),
        UI_Icon(0, 128 - 7, 0, 7, 7, BAGL_GLYPH_ICON_CHECK),
        UI_LabelLine(1, 0, 8, 128, 11, 0xFFFFFF, 0x000000, (const char *) viewctl_Title),
        UI_LabelLine(1, 0, 30, 128, 11, 0xFFFFFF, 0x000000, (const char *) viewctl_DataValue),
        UI_LabelLineScrolling(2, 16, 19, 96, 11, 0xFFFFFF, 0x000000, (const char *) viewctl_DataKey),
};

const bagl_element_t *viewconf_bagl_prepro(const bagl_element_t *element) {

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

static unsigned int viewconf_bagl_keyscrolling_button(
        unsigned int button_mask,
        unsigned int button_mask_counter) {
    switch (button_mask) {
        // Press left to progress to the previous element
        case BUTTON_EVT_RELEASED | BUTTON_LEFT: {
            viewctl_ehReject();
            break;
        }

            // Press right to progress to the next element
        case BUTTON_EVT_RELEASED | BUTTON_RIGHT: {
            viewctl_ehAccept();
            break;
        }
    }
    return 0;
}

static unsigned int viewconf_bagl_valuescrolling_button(
        unsigned int button_mask,
        unsigned int button_mask_counter) {
    return viewconf_bagl_keyscrolling_button(button_mask, button_mask_counter);
}

void viewconf_display_ux() {
    if (viewctl_scrolling_mode == VALUE_SCROLLING) {
        UX_DISPLAY(viewconf_bagl_valuescrolling, viewconf_bagl_prepro);
    } else {
        UX_DISPLAY(viewconf_bagl_keyscrolling, viewconf_bagl_prepro);
    }
}

void viewconf_start(int start_page,
                    viewctl_delegate_getData func_update,
                    viewctl_delegate_ready func_ready,
                    viewctl_delegate_exit func_exit,
                    viewctl_delegate_accept func_accept,
                    viewctl_delegate_reject func_reject) {
    viewctl_ehAccept = func_accept;
    viewctl_ehReject = func_reject;
    viewctl_start(start_page, func_update, func_ready, func_exit, viewconf_display_ux);
}
