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
#include "crypto.h"

#include <string.h>
#include <stdio.h>

viewctl_delegate_getData ehGetData = NULL;
viewctl_delegate_accept ehAccept = NULL;
viewctl_delegate_reject ehReject = NULL;

void accept(unsigned int _) {
    UNUSED(_);
    if (ehAccept != NULL) ehAccept();
}

void reject(unsigned int _) {
    UNUSED(_);
    if (ehReject != NULL) ehReject();
}

#define MAX_VAL(a, b) ( (a)>(b) ? (a) : (b) )
#define MIN_VAL(a, b) ( (a)<(b) ? (a) : (b) )

#define VIEW_ADDR_MODE_ACCOUNT 0
#define VIEW_ADDR_MODE_INDEX 1
#define VIEW_ADDR_MODE_SHOW 2

void view_addr_choose_refresh();

void view_addr_choose_update();

struct {
    // modes
    // 0 - select account
    // 1 - select index
    uint8_t mode;
    // TODO: Move this to bech32
    uint32_t account;
    uint32_t index;
} view_addr_choose_data;


#if defined(TARGET_NANOX)

#include "ux.h"
ux_state_t G_ux;
bolos_ux_params_t G_ux_params;

union {
    struct {
        char account[40];
        char index[40];
        char bech32[200];
    } addr;
} view;

#ifdef TESTING_ENABLED
UX_FLOW_DEF_NOCB(ux_idle_flow_1_step, pbb, { &C_icon_app, "Tendermint", "Cosmos (TEST)", });
#else
UX_FLOW_DEF_NOCB(ux_idle_flow_1_step, pbb, { &C_icon_app, "Tendermint", "Cosmos", });
#endif
UX_FLOW_DEF_NOCB(ux_idle_flow_2_step, bn, { "Version", APPVERSION, });
UX_FLOW_DEF_VALID(ux_idle_flow_3_step, pb, os_sched_exit(-1), { &C_icon_dashboard, "Quit",});
const ux_flow_step_t *const ux_idle_flow [] = {
  &ux_idle_flow_1_step,
  &ux_idle_flow_2_step,
  &ux_idle_flow_3_step,
  FLOW_END_STEP,
};

UX_FLOW_DEF_VALID(ux_tx_flow_1_step, pbb, view_tx_show(0), { &C_icon_eye, "Review", "Transaction" });
UX_FLOW_DEF_VALID(ux_tx_flow_2_step, pbb, accept(0), { &C_icon_validate_14, "Sign", "Transaction" });
UX_FLOW_DEF_VALID(ux_tx_flow_3_step, pbb, reject(0), { &C_icon_crossmark, "Reject", "Transaction" });
const ux_flow_step_t *const ux_tx_flow [] = {
  &ux_tx_flow_1_step,
  &ux_tx_flow_2_step,
  &ux_tx_flow_3_step,
  FLOW_END_STEP,
};

UX_FLOW_DEF_NOCB(ux_addr_flow_1_step, bnn, { "Address Request", view.addr.account, view.addr.index});
UX_FLOW_DEF_NOCB(ux_addr_flow_2_step, bnnn_paging, { .title = "Address", .text = view.addr.bech32 });
UX_FLOW_DEF_VALID(ux_addr_flow_3_step, pb, accept(0), { &C_icon_validate_14, "Reply", });
UX_FLOW_DEF_VALID(ux_addr_flow_4_step, pb, reject(0), { &C_icon_crossmark, "Reject", });
const ux_flow_step_t *const ux_addr_flow [] = {
  &ux_addr_flow_1_step,
  &ux_addr_flow_2_step,
  &ux_addr_flow_3_step,
  &ux_addr_flow_4_step,
  FLOW_END_STEP,
};
#else

// Nano S
ux_state_t ux;

const ux_menu_entry_t menu_transaction_info[] = {
        {NULL, view_tx_show, 0, NULL, "View transaction", NULL, 0, 0},
        {NULL, accept, 0, NULL, "Sign transaction", NULL, 0, 0},
        {NULL, reject, 0, &C_icon_crossmark, "Reject", NULL, 60, 40},
        UX_MENU_END
};

const ux_menu_entry_t menu_main[] = {
#ifdef TESTING_ENABLED
        {NULL, NULL, 0, &C_icon_app, "Tendermint", "Cosmos TEST!", 33, 12},
#else
        {NULL, NULL, 0, &C_icon_app, "Tendermint", "Cosmos", 33, 12},
#endif
        {NULL, view_addr_choose_show, 0, NULL, "Show Address", NULL, 0, 0},
        {NULL, NULL, 0, NULL, "v"APPVERSION, NULL, 0, 0},
        {NULL, os_sched_exit, 0, &C_icon_dashboard, "Quit app", NULL, 50, 29},
        UX_MENU_END
};

#define UIID_ICONACCEPT 0x50
#define UIID_ICONREJECT 0x51

static const bagl_element_t view_addr_show[] = {
        UI_FillRectangle(0, 0, 0, UI_SCREEN_WIDTH, UI_SCREEN_HEIGHT, 0x000000, 0xFFFFFF),
        UI_Icon(UIID_ICONACCEPT, 0, 0, 7, 7, BAGL_GLYPH_ICON_CROSS),
        UI_Icon(UIID_ICONREJECT, 128 - 7, 0, 7, 7, BAGL_GLYPH_ICON_CHECK),
        UI_LabelLine(UIID_LABEL + 0, 0, 8, 128, 11, UI_WHITE, UI_BLACK, (const char *) viewctl.title),
        UI_LabelLine(UIID_LABEL + 1, 0, 19, 128, 11, UI_WHITE, UI_BLACK, (const char *) viewctl.dataKey),
        UI_LabelLineScrolling(UIID_LABELSCROLL, 16, 30, 96, 11, UI_WHITE, UI_BLACK, (const char *) viewctl.dataValue),
};

const bagl_element_t *view_addr_show_prepro(const bagl_element_t *element) {
    switch (element->component.userid) {
        case UIID_ICONACCEPT:
            if (ehReject==NULL)
                return NULL;
            break;
        case UIID_ICONREJECT:
            if (ehAccept==NULL)
                return NULL;
            break;
        case UIID_LABELSCROLL:
            UX_CALLBACK_SET_INTERVAL(MAX(3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
            break;
    }
    return element;
}

static unsigned int view_addr_show_button(
        unsigned int button_mask,
        unsigned int button_mask_counter) {
    switch (button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
            // Press left to progress to the previous element
            reject(0);
            break;
        case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
            // Press right to progress to the next element
            accept(0);
            break;
    }
    return 0;
}

#define UIID_ICONLEFT1  0x50
#define UIID_ICONRIGHT1 0x51
#define UIID_ICONLEFT2  0x52
#define UIID_ICONRIGHT2 0x53

static const bagl_element_t view_addr_choose[] = {
        UI_FillRectangle(0, 0, 0, UI_SCREEN_WIDTH, UI_SCREEN_HEIGHT, 0x000000, 0xFFFFFF),

        UI_Icon(UIID_ICONLEFT1, 0, 0, 7, 7, BAGL_GLYPH_ICON_LEFT),
        UI_Icon(UIID_ICONRIGHT1, 128-7, 0, 7, 7, BAGL_GLYPH_ICON_RIGHT),
        UI_Icon(UIID_ICONLEFT2, 0, 9, 7, 7, BAGL_GLYPH_ICON_LEFT),
        UI_Icon(UIID_ICONRIGHT2, 128-7, 9, 7, 7, BAGL_GLYPH_ICON_RIGHT),

        UI_LabelLine(UIID_LABEL + 0, 0, 8, 128, 11, UI_WHITE, UI_BLACK, (const char *) viewctl.title),
        UI_LabelLine(UIID_LABEL + 1, 0, 19, 128, 11, UI_WHITE, UI_BLACK, (const char *) viewctl.dataKey),
        UI_LabelLineScrolling(UIID_LABELSCROLL, 16, 30, 96, 11, UI_WHITE, UI_BLACK, (const char *) viewctl.dataValue),
};

const bagl_element_t *view_addr_choose_prepro(const bagl_element_t *element) {
    switch (element->component.userid) {
        case UIID_ICONLEFT1:
        case UIID_ICONRIGHT1:
            if (view_addr_choose_data.mode != VIEW_ADDR_MODE_ACCOUNT)
                return NULL;
            break;
        case UIID_ICONLEFT2:
        case UIID_ICONRIGHT2:
            if (view_addr_choose_data.mode != VIEW_ADDR_MODE_INDEX)
                return NULL;
            break;
        case UIID_LABELSCROLL:
            UX_CALLBACK_SET_INTERVAL(MAX(3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
            break;
    }
    return element;
}

static unsigned int view_addr_choose_button(unsigned int button_mask, unsigned int button_mask_counter) {
    switch (button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT:
            // Press both to accept / switch mode
            switch (view_addr_choose_data.mode) {
                case VIEW_ADDR_MODE_ACCOUNT:
                    view_addr_choose_data.mode = VIEW_ADDR_MODE_INDEX;
                    break;
                case VIEW_ADDR_MODE_INDEX:
                    bip32_depth = 5;
                    bip32_path[0] = BIP32_0_DEFAULT;
                    bip32_path[1] = BIP32_1_DEFAULT;
                    bip32_path[2] = 0x80000000 | view_addr_choose_data.account;
                    bip32_path[3] = BIP32_3_DEFAULT;
                    bip32_path[4] = view_addr_choose_data.index;
                    view_addr_confirm(0);
                    return 0;
            }
            break;

        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
            // Press left -> previous element
            switch (view_addr_choose_data.mode) {
                case VIEW_ADDR_MODE_ACCOUNT:
                    view_addr_choose_data.account--;
                    break;
                case VIEW_ADDR_MODE_INDEX:
                    view_addr_choose_data.index--;
                    break;
            }
            break;

        case BUTTON_EVT_FAST | BUTTON_LEFT:
            // Hold left -> previous element (fast)
            switch (view_addr_choose_data.mode) {
                case VIEW_ADDR_MODE_ACCOUNT:
                    view_addr_choose_data.account -= 10;
                    break;
                case VIEW_ADDR_MODE_INDEX:
                    view_addr_choose_data.index -= 10;
                    break;
            }
            break;

        case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
            // Press right -> next element
            switch (view_addr_choose_data.mode) {
                case VIEW_ADDR_MODE_ACCOUNT:
                    view_addr_choose_data.account++;
                    break;
                case VIEW_ADDR_MODE_INDEX:
                    view_addr_choose_data.index++;
                    break;
            }
            break;

        case BUTTON_EVT_FAST | BUTTON_RIGHT:
            // Press right -> next element (fast)
            switch (view_addr_choose_data.mode) {
                case VIEW_ADDR_MODE_ACCOUNT:
                    view_addr_choose_data.account += 10;
                    break;
                case VIEW_ADDR_MODE_INDEX:
                    view_addr_choose_data.index += 10;
                    break;
            }
            break;

        default:
            return 0;
    }

    view_addr_choose_update();
    view_addr_choose_refresh();
    return 0;
}

#endif

////////////////////////////////
////////////////////////////////
////////////////////////////////

void io_seproxyhal_display(const bagl_element_t *element) {
    io_seproxyhal_display_default((bagl_element_t *) element);
}

void view_init(void) {
    UX_INIT();
}

void view_idle(unsigned int ignored) {
#if defined(TARGET_NANOS)
    UX_MENU_DISPLAY(0, menu_main, NULL);
#elif defined(TARGET_NANOX)
    if(G_ux.stack_count == 0) {
        ux_stack_push();
    }
    ux_flow_init(0, ux_idle_flow, NULL);
#endif
}

void view_tx_show(unsigned int start_page) {
    if (ehGetData == NULL) { return; }
    viewexpl_start(start_page,
                   ehGetData,
                   NULL,
                   view_tx_menu);

}

void view_addr_choose_update() {
    print_title("Account %u", view_addr_choose_data.account);
    print_key("Index %u", view_addr_choose_data.index);
    print_value("...");
}

void view_addr_choose_refresh() {
#if defined(TARGET_NANOS)
    UX_DISPLAY(view_addr_choose, view_addr_choose_prepro);
#elif defined(TARGET_NANOX)
    if(G_ux.stack_count == 0) {
        ux_stack_push();
    }
    ux_flow_init(0, ux_addr_flow, NULL);
#endif
}

void show_idle_menu() {
    view_idle(0);
}

void view_addr_choose_show(unsigned int _) {
    print_title("");
    print_key("");
    print_value("");
    snprintf(bech32_hrp, MAX_BECH32_HRP_LEN, "cosmos");
///
    ehAccept = show_idle_menu;
    ehReject = NULL;

    view_addr_choose_data.mode = VIEW_ADDR_MODE_ACCOUNT;
    view_addr_choose_data.account = 0;
    view_addr_choose_data.index = 0;
    view_addr_choose_update();
    view_addr_choose_refresh();
}

void view_addr_confirm(unsigned int _) {
#if defined(TARGET_NANOS)
    print_title("Account %u", BIP32_ACCOUNT);
    print_key("Index %u", BIP32_INDEX);
    print_value("....?....");
    UX_DISPLAY(view_addr_show, view_addr_show_prepro);
    UX_WAIT();

    get_bech32_addr(viewctl.dataValue);
    UX_DISPLAY(view_addr_show, view_addr_show_prepro);
#elif defined(TARGET_NANOX)
    snprintf(view.addr.account, sizeof(view.addr.account), "Account %u", BIP32_ACCOUNT);
    snprintf(view.addr.index, sizeof(view.addr.index), "Index %u", BIP32_INDEX);
    get_bech32_addr(view.addr.bech32);

    if(G_ux.stack_count == 0) {
        ux_stack_push();
    }

    ux_flow_init(0, ux_addr_flow, NULL);
#endif
}

void view_tx_menu(unsigned int unused) {
    UNUSED(unused);

#if defined(TARGET_NANOS)
    UX_MENU_DISPLAY(0, menu_transaction_info, NULL);
#elif defined(TARGET_NANOX)
    if(G_ux.stack_count == 0) {
        ux_stack_push();
    }
    ux_flow_init(0, ux_tx_flow, NULL);
#endif
}

void view_set_handlers(viewctl_delegate_getData func_getData,
                       viewctl_delegate_accept func_accept,
                       viewctl_delegate_reject func_reject) {
    ehGetData = func_getData;
    ehAccept = func_accept;
    ehReject = func_reject;
}
