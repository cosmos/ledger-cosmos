/*******************************************************************************
*  (c) 2019 ZondaX GmbH
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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

typedef enum {
    parser_ok = 0,
    parser_no_data,
    parser_init_context_empty,
    parser_unexpected_buffer_end,
    parser_unexpected_version,
    parser_unexpected_characters,
    parser_unexpected_field,
    parser_duplicated_field,
    parser_value_out_of_range,
    parser_unexpected_chain,
    parser_query_no_results,
    /////
    parser_display_idx_out_of_range,
    parser_display_page_out_of_range,
    ////
    parser_json_zero_tokens,
    parser_json_too_many_tokens,    // "NOMEM: JSON string contains too many tokens"
    parser_json_incomplete_json,    // "JSON string is not complete";
    parser_json_contains_whitespace,
    parser_json_is_not_sorted,
    parser_json_missing_chain_id,
    parser_json_missing_sequence,
    parser_json_missing_fee,
    parser_json_missing_msgs,
    parser_json_missing_account_number,
    parser_json_missing_memo,
} parser_error_t;

typedef struct {
    const uint8_t *buffer;
    uint16_t bufferLen;
    uint16_t offset;
} parser_context_t;

#ifdef __cplusplus
}
#endif
