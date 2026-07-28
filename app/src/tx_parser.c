/*******************************************************************************
 *   (c) 2018 - 2023 Zondax AG
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
#ifdef __cplusplus
#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
#endif

#include "tx_parser.h"
#include "parser_impl.h"
#include "zxformat.h"
#include "zxmacros.h"
#include <jsmn.h>

// strcat but source does not need to be terminated (a chunk from a bigger
// string is concatenated) dst_max is measured in bytes including the space for
// NULL termination src_size does not include NULL termination
__Z_INLINE void strcat_chunk_s(char *dst, uint16_t dst_max,
                               const char *src_chunk, size_t src_chunk_size) {
  if (dst == NULL || dst_max == 0) {
    return;
  }

  *(dst + dst_max - 1) =
      0; // last character terminates with zero in case we go beyond bounds
  const size_t prev_size = strlen(dst);

  size_t space_left =
      dst_max - prev_size - 1; // -1 because requires termination

  if (src_chunk_size > space_left) {
    src_chunk_size = space_left;
  }

  // Check bounds
  if (src_chunk_size > 0) {
    MEMMOVE(dst + prev_size, src_chunk, src_chunk_size);
    *(dst + prev_size + src_chunk_size) = 0;
  }
}

///////////////////////////
///////////////////////////
///////////////////////////
///////////////////////////
///////////////////////////
///////////////////////////
///////////////////////////

static const key_subst_t value_substitutions[] = {
    {"cosmos-sdk/MsgSend", "Send"},
    {"cosmos-sdk/MsgDelegate", "Delegate"},
    {"cosmos-sdk/MsgUndelegate", "Undelegate"},
    {"cosmos-sdk/MsgBeginRedelegate", "Redelegate"},
    {"cosmos-sdk/MsgSubmitProposal", "Propose"},
    {"cosmos-sdk/MsgDeposit", "Deposit"},
    {"cosmos-sdk/MsgVote", "Vote"},
    {"cosmos-sdk/MsgWithdrawDelegationReward", "Withdraw Reward"},
    {"cosmos-sdk/MsgWithdrawValidatorCommission", "Withdraw Val. Commission"},
    {"cosmos-sdk/MsgSetWithdrawAddress", "Withdraw Set Address"},
    {"cosmos-sdk/MsgMultiSend", "Multi Send"},

    // Babylon x/epoching wrapped staking messages. Each nests a standard Cosmos
    // staking message under a "msg" key (see tx_msg_max_level); the action
    // shown is the wrapped staking action. Ledger Live emits the amino `type`
    // as the proto type URL (/babylon.epoching.v1.MsgWrapped*); the chain's
    // x/epoching amino codec also registers the short legacy names
    // (epoching/Wrapped*). Accept both forms.
    {"/babylon.epoching.v1.MsgWrappedDelegate", "Delegate"},
    {"/babylon.epoching.v1.MsgWrappedUndelegate", "Undelegate"},
    {"/babylon.epoching.v1.MsgWrappedBeginRedelegate", "Redelegate"},
    {"epoching/WrappedDelegate", "Delegate"},
    {"epoching/WrappedUndelegate", "Undelegate"},
    {"epoching/WrappedBeginRedelegate", "Redelegate"},

};

parser_error_t tx_getToken(uint16_t token_index, char *out_val,
                           uint16_t out_val_len, uint8_t pageIdx,
                           uint8_t *pageCount) {
  if (pageCount == NULL) {
    return parser_unexpected_value;
  }

  *pageCount = 0;
  MEMZERO(out_val, out_val_len);

  const int16_t token_start =
      parser_tx_obj.tx_json.json.tokens[token_index].start;
  const int16_t token_end = parser_tx_obj.tx_json.json.tokens[token_index].end;

  if (token_start > token_end) {
    return parser_unexpected_buffer_end;
  }

  const char *inValue = parser_tx_obj.tx_json.tx + token_start;
  uint16_t inLen = token_end - token_start;

  // empty strings are considered the first page
  *pageCount = 1;
  if (inLen > 0) {
    // Only msgs/N/type carries an amino type name. Substituting anywhere else
    // lets an attacker-chosen memo or address render as an action word.
    if (is_msg_type_field(parser_tx_obj.tx_json.query.out_key)) {
      for (uint32_t i = 0; i < array_length(value_substitutions); i++) {
        const char *str1 = (const char *)PIC(value_substitutions[i].str1);
        const char *str2 = (const char *)PIC(value_substitutions[i].str2);
        const uint16_t str1Len = strlen(str1);
        const uint16_t str2Len = strlen(str2);

        if (inLen == str1Len && strncmp(inValue, str1, str1Len) == 0) {
          inValue = str2;
          inLen = str2Len;
          break;
        }
      }
    }
    pageStringExt(out_val, out_val_len, inValue, inLen, pageIdx, pageCount);
  }

  if (pageIdx >= *pageCount) {
    return parser_display_page_out_of_range;
  }

  return parser_ok;
}

__Z_INLINE void append_key_item(uint16_t token_index) {
  if (*parser_tx_obj.tx_json.query.out_key > 0) {
    // There is already something there, add separator
    strcat_chunk_s(parser_tx_obj.tx_json.query.out_key,
                   parser_tx_obj.tx_json.query.out_key_len, "/", 1);
  }

  const int16_t token_start =
      parser_tx_obj.tx_json.json.tokens[token_index].start;
  const int16_t token_end = parser_tx_obj.tx_json.json.tokens[token_index].end;
  const char *address_ptr = parser_tx_obj.tx_json.tx + token_start;
  const int32_t new_item_size = token_end - token_start;

  strcat_chunk_s(parser_tx_obj.tx_json.query.out_key,
                 parser_tx_obj.tx_json.query.out_key_len, address_ptr,
                 new_item_size);
}
///////////////////////////
///////////////////////////
///////////////////////////
///////////////////////////
///////////////////////////
///////////////////////////

__Z_INLINE bool msg_type_equals(const char *type_str, size_t type_len,
                                const char *name) {
  return type_len == strlen(name) && strncmp(type_str, name, type_len) == 0;
}

uint8_t tx_msg_max_level(uint16_t msg_token_index) {
  // Return the flatten depth needed to display this single message, based on
  // its own "type". Two shapes nest one object level deeper than the base and
  // need the extra level:
  //   - cosmos-sdk/MsgMultiSend (value-wrapped, nesting coins under
  //     inputs/outputs);
  //   - Babylon x/epoching wrapped staking messages, which nest a standard
  //     staking message under a "msg" key. The amino `type` is either the proto
  //     type URL (/babylon.epoching.v1.MsgWrapped*, as Ledger Live emits) or
  //     the legacy codec name (epoching/Wrapped*).
  // The legacy shape carries no "type" field and displays correctly at the base
  // level.
  uint16_t type_token_index = 0;
  if (object_get_value(&parser_tx_obj.tx_json.json, msg_token_index, "type",
                       &type_token_index) != parser_ok) {
    return MSG_BASE_FLATTEN_LEVEL;
  }

  const int16_t start =
      parser_tx_obj.tx_json.json.tokens[type_token_index].start;
  const int16_t end = parser_tx_obj.tx_json.json.tokens[type_token_index].end;
  if (start < 0 || end < start) {
    return MSG_BASE_FLATTEN_LEVEL;
  }

  const char *type_str = parser_tx_obj.tx_json.tx + start;
  const size_t type_len = (size_t)(end - start);
  if (msg_type_equals(type_str, type_len, "cosmos-sdk/MsgMultiSend")) {
    return MSG_MULTISEND_FLATTEN_LEVEL;
  }
  if (msg_type_equals(type_str, type_len,
                      "/babylon.epoching.v1.MsgWrappedDelegate") ||
      msg_type_equals(type_str, type_len,
                      "/babylon.epoching.v1.MsgWrappedUndelegate") ||
      msg_type_equals(type_str, type_len,
                      "/babylon.epoching.v1.MsgWrappedBeginRedelegate") ||
      msg_type_equals(type_str, type_len, "epoching/WrappedDelegate") ||
      msg_type_equals(type_str, type_len, "epoching/WrappedUndelegate") ||
      msg_type_equals(type_str, type_len, "epoching/WrappedBeginRedelegate")) {
    return MSG_EPOCHING_FLATTEN_LEVEL;
  }
  return MSG_BASE_FLATTEN_LEVEL;
}

parser_error_t tx_traverse_find(uint16_t root_token_index,
                                uint16_t *ret_value_token_index) {
  const jsmntype_t token_type =
      parser_tx_obj.tx_json.json.tokens[root_token_index].type;

  CHECK_APP_CANARY()

  if (parser_tx_obj.tx_json.tx == NULL) {
    return parser_no_data;
  }

  if (parser_tx_obj.tx_json.query.max_level <= 0 ||
      parser_tx_obj.tx_json.query.max_depth <= 0 || token_type == JSMN_STRING ||
      token_type == JSMN_PRIMITIVE) {
    const bool skipTypeField =
        parser_tx_obj.tx_json.flags.cache_valid &&
        parser_tx_obj.tx_json.flags.msg_type_grouping &&
        is_msg_type_field(parser_tx_obj.tx_json.query.out_key) &&
        parser_tx_obj.tx_json.filter_msg_type_valid_idx !=
            parser_tx_obj.tx_json.query._item_index_current;

    const bool skipFromFieldHidingRule =
        parser_tx_obj.tx_json.flags.msg_from_grouping_hide_all ||
        parser_tx_obj.tx_json.filter_msg_from_valid_idx !=
            parser_tx_obj.tx_json.query._item_index_current;

    const bool skipFromField =
        parser_tx_obj.tx_json.flags.cache_valid &&
        parser_tx_obj.tx_json.flags.msg_from_grouping &&
        is_msg_from_field(parser_tx_obj.tx_json.query.out_key) &&
        skipFromFieldHidingRule;

    const bool skipField = skipFromField || skipTypeField;

    CHECK_APP_CANARY()

    // Early bail out
    if (!skipField && parser_tx_obj.tx_json.query._item_index_current ==
                          parser_tx_obj.tx_json.query.item_index) {
      *ret_value_token_index = root_token_index;
      CHECK_APP_CANARY()
      return parser_ok;
    }

    if (skipField) {
      parser_tx_obj.tx_json.query.item_index++;
    }

    parser_tx_obj.tx_json.query._item_index_current++;
    CHECK_APP_CANARY()
    return parser_query_no_results;
  }

  uint16_t el_count;
  parser_error_t err;

  CHECK_PARSER_ERR(object_get_element_count(&parser_tx_obj.tx_json.json,
                                            root_token_index, &el_count))

  switch (token_type) {
  case JSMN_OBJECT: {
    const size_t key_len = strlen(parser_tx_obj.tx_json.query.out_key);
    for (uint16_t i = 0; i < el_count; ++i) {
      uint16_t key_index;
      uint16_t value_index;

      CHECK_PARSER_ERR(object_get_nth_key(&parser_tx_obj.tx_json.json,
                                          root_token_index, i, &key_index))
      CHECK_PARSER_ERR(object_get_nth_value(&parser_tx_obj.tx_json.json,
                                            root_token_index, i, &value_index))

      // Skip writing keys if we are actually exploring to count
      append_key_item(key_index);
      CHECK_APP_CANARY()

      // When traversing objects both level and depth should be considered
      parser_tx_obj.tx_json.query.max_level--;
      parser_tx_obj.tx_json.query.max_depth--;

      // Traverse the value, extracting subkeys
      err = tx_traverse_find(value_index, ret_value_token_index);
      CHECK_APP_CANARY()
      parser_tx_obj.tx_json.query.max_level++;
      parser_tx_obj.tx_json.query.max_depth++;

      if (err == parser_ok) {
        return parser_ok;
      }

      *(parser_tx_obj.tx_json.query.out_key + key_len) = 0;
      CHECK_APP_CANARY()
    }
    break;
  }
  case JSMN_ARRAY: {
    // Detect the top-level `msgs` array (uniquely identified by out_key ==
    // "msgs"; nested arrays are "msgs/value/inputs" etc.). Each element is a
    // message whose required flatten depth depends on its own type, so set
    // max_level per message from that type instead of using one depth for the
    // whole array. This way a batch that mixes message types (for example a
    // MsgMultiSend together with a MsgSend) enumerates the same set of fields
    // when counting items as when rendering them. Nested arrays keep the
    // inherited level unchanged.
    const bool is_msgs_array =
        strcmp(parser_tx_obj.tx_json.query.out_key, "msgs") == 0;
    const uint8_t saved_max_level = parser_tx_obj.tx_json.query.max_level;

    for (uint16_t i = 0; i < el_count; ++i) {
      uint16_t element_index;
      CHECK_PARSER_ERR(array_get_nth_element(
          &parser_tx_obj.tx_json.json, root_token_index, i, &element_index))
      CHECK_APP_CANARY()

      if (is_msgs_array) {
        parser_tx_obj.tx_json.query.max_level = tx_msg_max_level(element_index);
      }

      // When iterating along an array,
      // the level does not change but we need to count the recursion
      parser_tx_obj.tx_json.query.max_depth--;
      err = tx_traverse_find(element_index, ret_value_token_index);
      parser_tx_obj.tx_json.query.max_depth++;

      if (is_msgs_array) {
        parser_tx_obj.tx_json.query.max_level = saved_max_level;
      }

      CHECK_APP_CANARY()

      if (err == parser_ok) {
        return parser_ok;
      }
    }
    break;
  }
  default:
    break;
  }

  return parser_query_no_results;
}

#ifdef __cplusplus
#pragma clang diagnostic pop
#endif
