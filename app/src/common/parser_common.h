/*******************************************************************************
*  (c) 2019 Zondax GmbH
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

#define CHECK_PARSER_ERR(__CALL) { \
    parser_error_t __err = __CALL;  \
    CHECK_APP_CANARY()  \
    if (__err!=parser_ok) return __err;}

typedef enum {
    // Generic errors
    parser_ok = 0,
    parser_no_data,
    parser_init_context_empty,
    parser_display_idx_out_of_range,
    parser_display_page_out_of_range,
    parser_unexpected_error,
    // Coin generic
    parser_unexpected_type,
    parser_unexpected_method,
    parser_unexpected_buffer_end,
    parser_unexpected_value,
    parser_unexpected_number_items,
    parser_unexpected_version,
    parser_unexpected_characters,
    parser_unexpected_field,
    parser_duplicated_field,
    parser_value_out_of_range,
    parser_invalid_address,
    parser_unexpected_chain,
    parser_missing_field,
    parser_query_no_results,
    parser_transaction_too_big,
    // Coin Specific
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
    parser_json_unexpected_error,
    //cbor
    parser_cbor_unexpected,
    parser_cbor_unexpected_EOF,
    parser_cbor_not_canonical,
    // context
    parser_context_mismatch,
    parser_context_unexpected_size,
    parser_context_invalid_chars,
    parser_context_unknown_prefix,
} parser_error_t;

#ifdef __cplusplus
}
#endif
