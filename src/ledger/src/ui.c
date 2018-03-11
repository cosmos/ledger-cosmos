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
#include <string.h>
#include "ui.h"

ux_state_t ux;
enum UI_STATE uiState;

// {{type, userid, x, y, width, height, stroke, radius, fill, fgcolor, bgcolor, font_id, icon_id},
//   text, touch_area_brim, overfgcolor,  overbgcolor, tap, out, over, },
static const bagl_element_t bagl_ui_idle_nanos[] =
{
    {
        {BAGL_RECTANGLE, 0x00, 0, 0, 128, 32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF, 0, 0},
        NULL, 0, 0, 0, NULL, NULL, NULL,
    },
    {
        {BAGL_LABELINE, 0x02, 0, 12, 128, 11, 0, 0, 0, 0xFFFFFF, 0x000000, UI_CENTER11PX, 0},
        "Hello", 0, 0, 0, NULL, NULL, NULL,
    },
    {
        {BAGL_LABELINE, 0x02, 0, 23, 128, 11, 0, 0, 0, 0xFFFFFF, 0x000000, UI_CENTER11PX, 0},
        "World", 0, 0, 0, NULL, NULL, NULL,
    },
    {
        {BAGL_ICON, 0x00, 3, 12, 7,   7,  0, 0, 0, 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CROSS},
        NULL, 0, 0, 0, NULL, NULL, NULL,
    },
};


// {{type, userid, x, y, width, height, stroke, radius, fill, fgcolor, bgcolor, font_id, icon_id},
//   text, touch_area_brim, overfgcolor,  overbgcolor, tap, out, over, },
static const bagl_element_t bagl_ui_text_nanos[] =
{
        {
                {BAGL_RECTANGLE, 0x00, 0, 0, 128, 32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF, 0, 0},
                NULL, 0, 0, 0, NULL, NULL, NULL,
        },
        {
                {BAGL_LABELINE, 0x02, 0, 12, 128, 11, 0, 0, 0, 0xFFFFFF, 0x000000, UI_CENTER11PX, 0},
                "Czesc", 0, 0, 0, NULL, NULL, NULL,
        },
        {
                {BAGL_LABELINE, 0x02, 0, 23, 128, 11, 0, 0, 0, 0xFFFFFF, 0x000000, UI_CENTER11PX, 0},
                "Brawo", 0, 0, 0, NULL, NULL, NULL,
        },
        {
                {BAGL_ICON, 0x00, 3, 12, 7,   7,  0, 0, 0, 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CROSS},
                NULL, 0, 0, 0, NULL, NULL, NULL,
        },
};

void io_seproxyhal_display(const bagl_element_t *element) {
    io_seproxyhal_display_default((bagl_element_t *) element);
}

static const bagl_element_t *io_seproxyhal_touch_exit(const bagl_element_t *e) {
    os_sched_exit(0);   // Go back to the dashboard
    return NULL; // do not redraw the widget
}

static unsigned int bagl_ui_idle_nanos_button(unsigned int button_mask,
                                              unsigned int button_mask_counter) {
    switch (button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
        case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT:
            io_seproxyhal_touch_exit(NULL);
            break;
    }
    return 0;
}

static unsigned int bagl_ui_text_nanos_button(unsigned int button_mask,
                                              unsigned int button_mask_counter) {
    switch (button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
        case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT:
            io_seproxyhal_touch_exit(NULL);
            break;
    }
    return 0;
}

void ui_init(void)
{
    UX_INIT();
    uiState = UI_IDLE;
}

void ui_idle(void)
{
    uiState = UI_IDLE;
    UX_DISPLAY(bagl_ui_idle_nanos, NULL);
}

void ui_display_text(void)
{
    uiState = UI_IDLE;
    UX_DISPLAY(bagl_ui_text_nanos, NULL);
}