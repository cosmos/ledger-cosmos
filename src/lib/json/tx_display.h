/*******************************************************************************
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
#include <parser_common.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STRNCPY_S(DST, SRC, DST_SIZE) \
    explicit_bzero(DST, DST_SIZE);           \
    strncpy(DST, SRC, DST_SIZE - 1);

/// This is the main function called from ledger that updates key and value strings
/// that are going to be displayed in the UI.
/// This function assumes that the tx_ctx has been properly set
parser_error_t tx_display_set_query(uint16_t displayIdx, uint16_t *outStartToken);

/// Return number of UI pages that we'll have for the current json transaction (only if the tx is valid)
/// \return number of pages (msg pages + 5 required)
int16_t tx_display_numItems();

const char *get_required_root_item(uint8_t i);

void tx_display_make_friendly();

//---------------------------------------------

#ifdef __cplusplus
}
#endif
