/*******************************************************************************
*   (c) ZondaX GmbH
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

#include "json_parser.h"
#include "tx_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ERR_MUST_INDEX_FIRST -2

#define NUM_REQUIRED_ROOT_PAGES 6
#define NUM_KEY_SUBSTITUTIONS 29
#define NUM_VALUE_SUBSTITUTIONS 8

typedef struct {
    char str1[50];
    char str2[50];
} key_subst_t;

typedef struct {
    int16_t num_pages;
    int16_t subroot_start_token[NUM_REQUIRED_ROOT_PAGES];
    uint8_t num_subpages[NUM_REQUIRED_ROOT_PAGES];
} display_cache_t;

// This is only used for testing purposes
display_cache_t *tx_display_cache();

// This must be run before accessing items
void tx_display_index_root();

/// This is the main function called from ledger that updates key and value strings
/// that are going to be displayed in the UI.
/// This function assumes that the tx_ctx has been properly set
int16_t tx_display_get_item(uint16_t page_index);

/// Return number of UI pages that we'll have for the current json transaction (only if the tx is valid)
/// \return number of pages (msg pages + 5 required)
int16_t tx_display_num_pages();

/// Apply postprocessing rules to key and values
void tx_display_make_friendly();

//---------------------------------------------

#ifdef __cplusplus
}
#endif
