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
#include "lib/crypto.h"
#include "cosmos.h"

#include <string.h>
#include <stdio.h>

viewctl_delegate_getData ehGetData = NULL;
viewctl_delegate_accept ehAccept = NULL;
viewctl_delegate_reject ehReject = NULL;

#define MAX_VAL(a, b) ( (a)>(b) ? (a) : (b) )
#define MIN_VAL(a, b) ( (a)<(b) ? (a) : (b) )

#define VIEW_ADDR_MODE_ACCOUNT 0
#define VIEW_ADDR_MODE_INDEX 1
#define VIEW_ADDR_MODE_SHOW 2
#define VIEW_ADDR_MODE_CONFIRM 3

#define UIID_ICONACCEPT 0x50
#define UIID_ICONREJECT 0x51
#define UIID_ICONLEFT1  0x52
#define UIID_ICONRIGHT1 0x53
#define UIID_ICONLEFT2  0x54
#define UIID_ICONRIGHT2 0x55

#define UIID_MARKER1    0x60
#define UIID_MARKER2    0x61

void accept(unsigned int _) {
    UNUSED(_);
    if (ehAccept != NULL) ehAccept();
}

void reject(unsigned int _) {
    UNUSED(_);
    if (ehReject != NULL) ehReject();
}

void show_idle_menu() {
    view_idle(0);
}

void view_tx_menu(unsigned int _);

#if defined(TARGET_NANOX)

#include "ux.h"
ux_state_t G_ux;
bolos_ux_params_t G_ux_params;

#ifdef TESTING_ENABLED
UX_FLOW_DEF_NOCB(ux_idle_flow_1_step, pbb, { &C_icon_app, "Tendermint", "Cosmos TEST!", });
#else
UX_FLOW_DEF_NOCB(ux_idle_flow_1_step, pbb, { &C_icon_app, "Tendermint", "Cosmos", });
#endif
UX_FLOW_DEF_NOCB(ux_idle_flow_3_step, bn, { "Version", APPVERSION, });
UX_FLOW_DEF_VALID(ux_idle_flow_4_step, pb, os_sched_exit(-1), { &C_icon_dashboard, "Quit",});
const ux_flow_step_t *const ux_idle_flow [] = {
  &ux_idle_flow_1_step,
  &ux_idle_flow_2_step,
  &ux_idle_flow_3_step,
  &ux_idle_flow_4_step,
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

UX_FLOW_DEF_NOCB(ux_addr_flow_1_step, bnn, { "Address Request", viewctl.title, viewctl.dataKey});
UX_FLOW_DEF_NOCB(ux_addr_flow_2_step, bnnn_paging, { .title = "Address", .text = viewctl.dataValue });
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
        {NULL, NULL, 0, NULL, "v"APPVERSION, NULL, 0, 0},
        {NULL, os_sched_exit, 0, &C_icon_dashboard, "Quit app", NULL, 50, 29},
        UX_MENU_END
};

const ux_menu_entry_t menu_status[] = {
        {NULL, NULL, 0, &C_icon_app, viewctl.dataKey, viewctl.dataValue, 33, 12},
        UX_MENU_END
};

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

void view_status(unsigned int ignored) {
#if defined(TARGET_NANOS)
    UX_MENU_DISPLAY(0, menu_status, NULL);
#endif
}

void view_tx_show(unsigned int start_page) {
    if (ehGetData == NULL) { return; }
    viewexpl_start(start_page,
                   ehGetData,
                   NULL,
                   view_tx_menu);

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

const char *address;
void view_address_show() {
    // Address has been placed in the output buffer
    address = (char *) (G_io_apdu_buffer + PUBKEY_LEN);

    // FIXME:
    //view_address_show_impl();
}
