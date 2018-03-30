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
#include <stdio.h>
#include "ui.h"
#include "glyphs.h"
#include "ui_templates.h"
#include "bagl.h"

ux_state_t ux;
enum UI_STATE uiState;
unsigned int ux_step = 0;
unsigned int ux_step_count = 0;
unsigned int ux_total_size = 0;
unsigned int ux_direction = 0;

volatile char transactionDataName[32];
volatile char transactionDataValue[32];
volatile char pageInfo[10];

int transactionDetailsCurrentPage;
int transactionDetailsPageCount;

void start_transaction_info_display(unsigned int unused);
void sign_transaction(unsigned int unused);
void reject(unsigned int unused);

const ux_menu_entry_t menu_main[];
const ux_menu_entry_t menu_about[];

const ux_menu_entry_t menu_transaction_info[] = {
    {NULL, start_transaction_info_display, 0, NULL, "View transaction", NULL, 0, 0},
    {NULL, sign_transaction, 0, NULL, "Sign transaction", NULL, 0, 0},
    {NULL, reject, 0, &C_icon_back, "Reject", NULL, 60, 40},
    UX_MENU_END
};

const ux_menu_entry_t menu_main[] = {
    {NULL, NULL, 0, &C_icon_tendermint, "Tendermint", "Cosmos Demo", 33, 12},
    {menu_about, NULL, 0, NULL, "About", NULL, 0, 0},
    {NULL, os_sched_exit, 0, &C_icon_dashboard, "Quit app", NULL, 50, 29},
    UX_MENU_END
};

const ux_menu_entry_t menu_about[] = {
    {NULL, NULL, 0, NULL, "Version", APPVERSION, 0, 0},
    {menu_main, NULL, 2, &C_icon_back, "Back", NULL, 61, 40},
    UX_MENU_END
};

static const bagl_element_t bagl_ui_sign_transaction[] = {
    UI_FillRectangle(0, 0, 0, 128, 32, 0x000000, 0xFFFFFF),
    UI_Icon(0, 3, 32 / 2 - 4, 7, 7, BAGL_GLYPH_ICON_CROSS),
    UI_LabelLine(1, 0, 12, 128, 11, 0xFFFFFF, 0x000000, "Sign transaction"),
    UI_LabelLine(2, 0, 23, 128, 11, 0xFFFFFF, 0x000000, "Not implemented yet"),
};

static const bagl_element_t bagl_ui_transaction_info[] = {
    UI_FillRectangle(0, 0, 0, 128, 32, 0x000000, 0xFFFFFF),
    UI_Icon(0, 0, 0, 7, 7, BAGL_GLYPH_ICON_LEFT),
    UI_Icon(0, 128-7, 0, 7, 7, BAGL_GLYPH_ICON_RIGHT),
    UI_LabelLine(1, 0, 8, 128, 11, 0xFFFFFF, 0x000000,(const char*)pageInfo),
    UI_LabelLine(2, 0, 21, 128, 11, 0xFFFFFF, 0x000000,(const char*)transactionDataName),
    UI_LabelLine(3, 0, 32, 128, 11, 0xFFFFFF, 0x000000,(const char*)transactionDataValue),
};

UpdateTxDataPtr updateTxDataPtr = NULL;
RejectPtr rejectPtr = NULL;

void set_update_transaction_ui_data_callback(UpdateTxDataPtr ptr)
{
    updateTxDataPtr = ptr;
}

void set_reject_transaction_callback(RejectPtr ptr)
{
    rejectPtr = ptr;
}

static unsigned int bagl_ui_sign_transaction_button(unsigned int button_mask,
                                                    unsigned int button_mask_counter)
{
    switch (button_mask) {
        default:
            display_transaction_menu(0);
    }
    return 0;
}

const bagl_element_t* ui_transaction_info_prepro(const bagl_element_t *element) {

    if (element->component.userid == 0) {
        update_transaction_page_info();

        if (ux_total_size > MAX_CHARS_PER_LINE) {
            ux_step_count = ux_total_size - MAX_CHARS_PER_LINE;
            if (ux_step == 0 || ux_step == ux_step_count-1)   {
                UX_CALLBACK_SET_INTERVAL(2000);
            }
            else {
                UX_CALLBACK_SET_INTERVAL(500);
            }
        } else {
            ux_step_count = 0;
        }
    }

    return element;
}

void display_transaction_info()
{

}

static unsigned int bagl_ui_transaction_info_button(unsigned int button_mask,
                                                    unsigned int button_mask_counter)
{
    switch (button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT:
            display_transaction_menu(0);
            break;

        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
            if (transactionDetailsCurrentPage > 0) {
                transactionDetailsCurrentPage--;
                ux_step = 0;
                ux_direction = 0;
                UX_DISPLAY(bagl_ui_transaction_info, ui_transaction_info_prepro);
            } else {
                display_transaction_menu(0);
            }

            break;
        case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
            if (transactionDetailsCurrentPage < transactionDetailsPageCount - 1) {
                transactionDetailsCurrentPage++;
                ux_step = 0;
                ux_direction = 0;
                UX_DISPLAY(bagl_ui_transaction_info, ui_transaction_info_prepro);
            } else {
                display_transaction_menu(0);
            }
            break;
    }
    return 0;
}

void start_transaction_info_display(unsigned int unused)
{
    UNUSED(unused);
    transactionDetailsCurrentPage = 0;
    ux_step = 0;
    ux_direction = 0;
    UX_DISPLAY(bagl_ui_transaction_info, ui_transaction_info_prepro);
}

void update_transaction_page_info()
{
    updateTxDataPtr((char*)transactionDataName,
                    sizeof(transactionDataName),
                    (char*)transactionDataValue,
                    sizeof(transactionDataValue),
                    transactionDetailsCurrentPage);

    snprintf((char*)pageInfo, sizeof(pageInfo), "%d/%d", transactionDetailsCurrentPage+1, transactionDetailsPageCount);
}

void sign_transaction(unsigned int unused)
{
    cx_sha256_t sha2;
    uint8_t hashMessage[32];
    cx_sha256_init(&sha2);

    uint8_t workBuffer[100] = {0};
    int dataLength = sizeof(workBuffer);
    cx_hash((cx_hash_t *)&sha2, 0, workBuffer, dataLength, NULL);
    cx_hash((cx_hash_t *)&sha2, CX_LAST, workBuffer, 0, hashMessage);

    uint8_t pathLength;

    //uint8_t bip32Bufer[] = "44'/60'/0'/0";
    uint32_t bip32Path[] = {44, 60, 0, 0};
    pathLength = 4;

    uint8_t privateKeyData[32];
    uint8_t signature[100];
    uint8_t signatureLength;
    cx_ecfp_private_key_t privateKey;

    os_perso_derive_node_bip32(
            CX_CURVE_256K1,
            bip32Path,
            pathLength,
            privateKeyData,
            NULL);

    cx_ecfp_init_private_key(CX_CURVE_256K1, privateKeyData, 32, &privateKey);
    os_memset(privateKeyData, 0, sizeof(privateKeyData));

//#if CX_APILEVEL >= 8
//    unsigned int info = 0;
//    signatureLength =
//        cx_ecdsa_sign(&privateKey, CX_RND_RFC6979 | CX_LAST, CX_SHA256,
//                      tmpCtx.transactionContext.hash,
//                      sizeof(tmpCtx.transactionContext.hash), signature, &info);
//    if (info & CX_ECCINFO_PARITY_ODD) {
//        signature[0] |= 0x01;
//    }
//#else
    unsigned int info = 0;
    signatureLength =
            cx_ecdsa_sign(&privateKey, CX_RND_RFC6979 | CX_LAST, CX_SHA256,
                          hashMessage,
                          sizeof(hashMessage),
                          signature,
                          &info);
//#endif
    os_memset(&privateKey, 0, sizeof(privateKey));


    UNUSED(unused);
    UX_DISPLAY(bagl_ui_sign_transaction, NULL);
}

void reject(unsigned int unused)
{
    if (rejectPtr != NULL) {
        rejectPtr();
    }
}

void io_seproxyhal_display(const bagl_element_t *element)
{
    io_seproxyhal_display_default((bagl_element_t *) element);
}

void ui_init(void)
{
    UX_INIT();
    uiState = UI_IDLE;
}

void ui_idle(unsigned int ignored)
{
    uiState = UI_IDLE;
    UX_MENU_DISPLAY(0, menu_main, NULL);
}

void display_transaction_menu(unsigned int numberOfTransactionPages)
{
    if (numberOfTransactionPages != 0) {
        transactionDetailsPageCount = numberOfTransactionPages;
    }
    uiState = UI_TRANSACTION;
    UX_MENU_DISPLAY(0, menu_transaction_info, NULL);
}