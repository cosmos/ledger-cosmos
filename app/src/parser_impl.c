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

#include "parser_impl.h"
#include "cbor.h"
#include <cbor/cbor_parser_helper.h>

parser_tx_t parser_tx_obj;

const char *parser_getErrorDescription(parser_error_t err) {
  switch (err) {
  case parser_ok:
    return "No error";
  case parser_no_data:
    return "No more data";
  case parser_init_context_empty:
    return "Initialized empty context";
  case parser_unexpected_buffer_end:
    return "Unexpected buffer end";
  case parser_unexpected_version:
    return "Unexpected version";
  case parser_unexpected_characters:
    return "Unexpected characters";
  case parser_unexpected_field:
    return "Unexpected field";
  case parser_duplicated_field:
    return "Unexpected duplicated field";
  case parser_value_out_of_range:
    return "Value out of range";
  case parser_unexpected_chain:
    return "Unexpected chain";
  case parser_query_no_results:
    return "item query returned no results";
  case parser_missing_field:
    return "missing field";
  case parser_unexpected_type:
    return "unexpected type";
    //////
  case parser_display_idx_out_of_range:
    return "display index out of range";
  case parser_display_page_out_of_range:
    return "display page out of range";
    //////
  case parser_json_zero_tokens:
    return "JSON. Zero tokens";
  case parser_json_too_many_tokens:
    return "JSON. Too many tokens";
  case parser_json_incomplete_json:
    return "JSON string is not complete";
  case parser_json_contains_whitespace:
    return "JSON Contains whitespace in the corpus";
  case parser_json_is_not_sorted:
    return "JSON Dictionaries are not sorted";
  case parser_json_missing_chain_id:
    return "JSON Missing chain_id";
  case parser_json_missing_sequence:
    return "JSON Missing sequence";
  case parser_json_missing_fee:
    return "JSON Missing fee";
  case parser_json_missing_msgs:
    return "JSON Missing msgs";
  case parser_json_missing_account_number:
    return "JSON Missing account number";
  case parser_json_missing_memo:
    return "JSON Missing memo";
  case parser_json_unexpected_error:
    return "JSON Unexpected error";
  // cbor
  case parser_cbor_unexpected:
    return "unexpected CBOR error";
  case parser_cbor_not_canonical:
    return "CBOR was not in canonical order";
  case parser_cbor_unexpected_EOF:
    return "Unexpected CBOR EOF";
  // Context specific
  case parser_context_mismatch:
    return "context prefix is invalid";
  case parser_context_unexpected_size:
    return "context unexpected size";
  case parser_context_invalid_chars:
    return "context invalid chars";
  case parser_transaction_too_big:
    return "Transaction is too big";
  // swap
  case parser_swap_wrong_chain_id:
    return "Swap wrong chain id";
  case parser_swap_wrong_dest_address:
    return "Swap wrong destination address";
  case parser_swap_wrong_amount:
    return "Swap wrong amount";
  case parser_swap_wrong_dest_coins:
    return "Swap wrong destination coins";
  case parser_swap_wrong_memo:
    return "Swap wrong memo";
  case parser_swap_wrong_fee:
    return "Swap wrong fee";
  case parser_swap_unexpected_number_of_items:
    return "Swap unexpected number of items";
  case parser_swap_unexpected_field:
    return "Swap unexpected field";
  case parser_swap_wrap_amount_computation_error:
    return "Swap wrap amount computation error";
  case parser_swap_wrong_type:
    return "Swap wrong type";
  case parser_swap_memo_not_present:
    return "Swap memo not present";
  case parser_swap_wrong_source_coins:
    return "Swap wrong source coins";

  default:
    return "Unrecognized error code";
  }
}

parser_error_t _read_json_tx(parser_context_t *c, __Z_UNUSED parser_tx_t *v) {
  parser_error_t err = json_parse(&parser_tx_obj.tx_json.json,
                                  (const char *)c->buffer, c->bufferLen);
  if (err != parser_ok) {
    return err;
  }

  parser_tx_obj.tx_json.tx = (const char *)c->buffer;
  parser_tx_obj.tx_json.flags.cache_valid = 0;
  parser_tx_obj.tx_json.filter_msg_type_count = 0;
  parser_tx_obj.tx_json.filter_msg_from_count = 0;

  return parser_ok;
}

parser_error_t _read_text_tx(parser_context_t *c, parser_tx_t *v) {
#if !defined(COMPILE_TEXTUAL)
  UNUSED(c);
  UNUSED(v);
  return parser_value_out_of_range;
#else
  CborValue it;
  CborValue mapStruct_ptr;
  CHECK_APP_CANARY()
  INIT_CBOR_PARSER(c, mapStruct_ptr)
  CHECK_APP_CANARY()

  // Make sure we have a map/struct
  PARSER_ASSERT_OR_ERROR(cbor_value_is_map(&mapStruct_ptr),
                         parser_unexpected_type)
  CHECK_CBOR_MAP_ERR(cbor_value_enter_container(&mapStruct_ptr, &it))

  // Make sure we have screen_key set to 1
  int screen_key = 0;
  PARSER_ASSERT_OR_ERROR(cbor_value_is_integer(&it), parser_unexpected_type)
  CHECK_CBOR_MAP_ERR(cbor_value_get_int(&it, &screen_key))
  PARSER_ASSERT_OR_ERROR(screen_key == 1, parser_unexpected_type)
  CHECK_CBOR_MAP_ERR(cbor_value_advance(&it))

  // Make sure we have an array of containers and check size
  PARSER_ASSERT_OR_ERROR(cbor_value_is_array(&it), parser_unexpected_type)

  CHECK_CBOR_MAP_ERR(cbor_value_get_array_length(&it, &v->tx_text.n_containers))
  // Limit max fields to 255
  PARSER_ASSERT_OR_ERROR(
      (v->tx_text.n_containers > 0 && v->tx_text.n_containers <= UINT8_MAX),
      parser_unexpected_number_items)

  CborValue containerArray_ptr;
  CborValue data;
  Cbor_container container;

  // Enter array of containers
  CHECK_CBOR_MAP_ERR(cbor_value_enter_container(&it, &containerArray_ptr))

  for (size_t i = 0; i < v->tx_text.n_containers; i++) {
    MEMZERO(&container, sizeof(container));

    CHECK_CBOR_MAP_ERR(
        cbor_value_get_map_length(&containerArray_ptr, &container.n_field))
    PARSER_ASSERT_OR_ERROR((container.n_field > 0 && container.n_field < 5),
                           parser_unexpected_value)

    CHECK_CBOR_MAP_ERR(cbor_value_enter_container(&containerArray_ptr, &data))
    CHECK_PARSER_ERR(cbor_check_expert(&data, &container))

    v->tx_text.n_expert += container.screen.expert ? 1 : 0;
    CHECK_CBOR_MAP_ERR(cbor_value_advance(&containerArray_ptr))
  }
  CHECK_CBOR_MAP_ERR(cbor_value_leave_container(&it, &containerArray_ptr))

  // End of buffer does not match end of parsed data
  PARSER_ASSERT_OR_ERROR(it.source.ptr == c->buffer + c->bufferLen,
                         parser_cbor_unexpected_EOF)

  return parser_ok;
#endif
}
