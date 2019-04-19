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
#include "view_conf.h"
#include "common.h"

#include "glyphs.h"
#include "bagl.h"

#include <string.h>
#include <stdio.h>

viewctl_delegate_getData ehGetData = NULL;
viewctl_delegate_accept ehAccept = NULL;
viewctl_delegate_reject ehReject = NULL;

void accept(unsigned int unused);

void reject(unsigned int unused);

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
UX_FLOW_DEF_VALID(ux_tx_flow_2_step, pbb, view_sign_transaction(0), { &C_icon_validate_14, "Sign", "Transaction" });
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

//------ View elements
const ux_menu_entry_t menu_main[];
const ux_menu_entry_t menu_about[];

const ux_menu_entry_t menu_transaction_info[] = {
        {NULL, view_tx_show, 0, NULL, "View transaction", NULL, 0, 0},
        {NULL, view_sign_transaction, 0, NULL, "Sign transaction", NULL, 0, 0},
        {NULL, reject, 0, &C_icon_crossmark, "Reject", NULL, 60, 40},
        UX_MENU_END
};

const ux_menu_entry_t menu_main[] = {
#ifdef TESTING_ENABLED
        {NULL, NULL, 0, &C_icon_app, "Tendermint", "Cosmos TEST!", 33, 12},
#else
        {NULL, NULL, 0, &C_icon_app, "Tendermint", "Cosmos", 33, 12},
#endif
//        {NULL, view_addr_show, 0, NULL, "Test", NULL, 0, 0},
        {menu_about, NULL, 0, NULL, "About", NULL, 0, 0},
        {NULL, os_sched_exit, 0, &C_icon_dashboard, "Quit app", NULL, 50, 29},
        UX_MENU_END
};

const ux_menu_entry_t menu_about[] = {
        {NULL, NULL, 0, NULL, "Version", APPVERSION, 0, 0},
        {menu_main, NULL, 2, &C_icon_back, "Back", NULL, 61, 40},
        UX_MENU_END
};
#endif

////////////////////////////////
////////////////////////////////
////////////////////////////////

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
                   view_display_tx_menu);

}

void view_addr_exit(unsigned int unused) {
    G_io_apdu_buffer[0] = 0x90;
    G_io_apdu_buffer[1] = 0x00;
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
    view_idle(0);
}

void view_addr_confirm(unsigned int start_page) {
#if defined(TARGET_NANOS)
    viewconf_start(start_page,
                   ehGetData,   // update
                   NULL,        // ready
                   NULL,        // exit
                   ehAccept,
                   ehReject);
#elif defined(TARGET_NANOX)
    // Retrieve data
    ehGetData(view.addr.account, sizeof(view.addr.account),
              view.addr.index, sizeof(view.addr.index),
              view.addr.bech32, sizeof(view.addr.bech32),
              start_page, 0, 0, 0);

    if(G_ux.stack_count == 0) {
        ux_stack_push();
    }

    ux_flow_init(0, ux_addr_flow, NULL);
#endif
}

void view_sign_transaction(unsigned int unused) {
    accept(unused);
}

void reject(unsigned int unused) {
    UNUSED(unused);
    if (ehReject != NULL) {
        ehReject();
    }
}

void accept(unsigned int unused) {
    UNUSED(unused);
    if (ehAccept != NULL) {
        ehAccept();
    }
}

void view_display_tx_menu(unsigned int unused) {
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

void io_seproxyhal_display(const bagl_element_t *element) {
    io_seproxyhal_display_default((bagl_element_t *) element);
}
