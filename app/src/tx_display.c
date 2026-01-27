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

#include "tx_display.h"
#include "app_mode.h"
#include "coin.h"
#include "parser_impl.h"
#include "tx_parser.h"
#include "utf8.h"
#include <zxformat.h>
#include <zxmacros.h>

#define NUM_REQUIRED_ROOT_PAGES 7

// Hex escape "\xNN" requires 4 characters, but snprintf adds a null terminator
#define HEX_ESCAPE_LEN 4
#define HEX_ESCAPE_SNPRINTF_SIZE (HEX_ESCAPE_LEN + 1)

#define ASSERT_PTR_BOUNDS(count, dstLen)                                       \
  count++;                                                                     \
  if (count > dstLen) {                                                        \
    return parser_transaction_too_big;                                         \
  }

const char *get_required_root_item(root_item_e i) {
  switch (i) {
  case root_item_chain_id:
    return "chain_id";
  case root_item_account_number:
    return "account_number";
  case root_item_sequence:
    return "sequence";
  case root_item_fee:
    return "fee";
  case root_item_memo:
    return "memo";
  case root_item_msgs:
    return "msgs";
  case root_item_tip:
    return "tip";
  default:
    return "?";
  }
}

#ifdef __cplusplus
#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-branch-clone"
#endif

__Z_INLINE uint8_t get_root_max_level(root_item_e i) {
  switch (i) {
  case root_item_chain_id:
    return 2;
  case root_item_account_number:
    return 2;
  case root_item_sequence:
    return 2;
  case root_item_fee:
    return 1;
  case root_item_memo:
    return 2;
  case root_item_msgs:
    return extraDepthLevel ? 3 : 2;
  case root_item_tip:
    return 1;
  default:
    return 0;
  }
}

#ifdef __cplusplus
#pragma clang diagnostic pop
#endif

typedef struct {
  bool root_item_start_token_valid[NUM_REQUIRED_ROOT_PAGES];
  // token where the root_item starts (negative for non-existing)
  uint16_t root_item_start_token_idx[NUM_REQUIRED_ROOT_PAGES];

  // total items
  uint16_t total_item_count;
  // number of items the root_item contains
  uint8_t root_item_number_subitems[NUM_REQUIRED_ROOT_PAGES];

  uint8_t is_default_chain;
} display_cache_t;

display_cache_t display_cache;

__Z_INLINE parser_error_t calculate_is_default_chainid() {
  display_cache.is_default_chain = false;

  // get chain_id
  char outKey[2];
  char outVal[COIN_MAX_CHAINID_LEN];
  uint8_t pageCount;
  INIT_QUERY_CONTEXT(outKey, sizeof(outKey), outVal, sizeof(outVal), 0,
                     get_root_max_level(root_item_chain_id))
  parser_tx_obj.tx_json.query.item_index = 0;
  parser_tx_obj.tx_json.query._item_index_current = 0;

  uint16_t ret_value_token_index;
  CHECK_PARSER_ERR(tx_traverse_find(
      display_cache.root_item_start_token_idx[root_item_chain_id],
      &ret_value_token_index))

  CHECK_PARSER_ERR(
      tx_getToken(ret_value_token_index, outVal, sizeof(outVal), 0, &pageCount))

  zemu_log_stack(outVal);
  zemu_log_stack(COIN_DEFAULT_CHAINID);

  if (strcmp(outVal, COIN_DEFAULT_CHAINID) == 0) {
    // If we don't match the default chainid, switch to expert mode
    display_cache.is_default_chain = true;
    zemu_log_stack("DEFAULT Chain ");
  } else if ((outVal[0] == 0x30 || outVal[0] == 0x31) && strlen(outVal) == 1) {
    zemu_log_stack("Not Allowed chain");
    return parser_unexpected_chain;
  } else {
    zemu_log_stack("Chain is NOT DEFAULT");
  }

  return parser_ok;
}

__Z_INLINE bool address_matches_own(char *addr) {
  if (parser_tx_obj.tx_json.own_addr == NULL) {
    return false;
  }

  // Validate length to prevent buffer over-read
  if (parser_tx_obj.tx_json.own_addr_len > MAX_BECH32_ADDR_LEN) {
    return false;
  }

  // own_addr_len is the exact length of the address (may not be
  // null-terminated)
  if (strlen(addr) != parser_tx_obj.tx_json.own_addr_len) {
    return false;
  }

  if (strncmp(parser_tx_obj.tx_json.own_addr, addr,
              parser_tx_obj.tx_json.own_addr_len) != 0) {
    return false;
  }
  return true;
}

parser_error_t tx_indexRootFields() {
  if (parser_tx_obj.tx_json.flags.cache_valid) {
    return parser_ok;
  }

#ifdef APP_TESTING
  zemu_log("tx_indexRootFields");
#endif

  // Clear cache
  MEMZERO(&display_cache, sizeof(display_cache_t));

  char tmp_key[INDEXING_TMP_KEYSIZE];
  char tmp_val[INDEXING_TMP_VALUESIZE];
  MEMZERO(&tmp_key, sizeof(tmp_key));
  MEMZERO(&tmp_val, sizeof(tmp_val));

  // Grouping references
  char reference_msg_type[INDEXING_GROUPING_REF_TYPE_SIZE];
  char reference_msg_from[INDEXING_GROUPING_REF_FROM_SIZE];
  MEMZERO(&reference_msg_type, sizeof(reference_msg_type));
  MEMZERO(&reference_msg_from, sizeof(reference_msg_from));

  parser_tx_obj.tx_json.filter_msg_type_count = 0;
  parser_tx_obj.tx_json.filter_msg_from_count = 0;
  parser_tx_obj.tx_json.flags.msg_type_grouping = 1;
  parser_tx_obj.tx_json.flags.msg_from_grouping = 1;

  // Look for all expected root items in the JSON tree
  // mark them as found/valid,

  for (root_item_e root_item_idx = 0; root_item_idx < NUM_REQUIRED_ROOT_PAGES;
       root_item_idx++) {
    uint16_t req_root_item_key_token_idx = 0;

    const char *required_root_item_key = get_required_root_item(root_item_idx);

    parser_error_t err =
        object_get_value(&parser_tx_obj.tx_json.json, ROOT_TOKEN_INDEX,
                         required_root_item_key, &req_root_item_key_token_idx);

    if (err == parser_no_data) {
      continue;
    }
    CHECK_PARSER_ERR(err)

    // Remember root item start token
    display_cache.root_item_start_token_valid[root_item_idx] = true;
    display_cache.root_item_start_token_idx[root_item_idx] =
        req_root_item_key_token_idx;

    // Now count how many items can be found in this root item
    int16_t current_item_idx = 0;
    while (err == parser_ok) {
      INIT_QUERY_CONTEXT(tmp_key, sizeof(tmp_key), tmp_val, sizeof(tmp_val), 0,
                         get_root_max_level(root_item_idx))

      parser_tx_obj.tx_json.query.item_index = current_item_idx;
      strncpy_s(parser_tx_obj.tx_json.query.out_key, required_root_item_key,
                parser_tx_obj.tx_json.query.out_key_len);

      uint16_t ret_value_token_index;
      err = tx_traverse_find(
          display_cache.root_item_start_token_idx[root_item_idx],
          &ret_value_token_index);
      if (err != parser_ok) {
        continue;
      }

      uint8_t pageCount;
      CHECK_PARSER_ERR(tx_getToken(
          ret_value_token_index, parser_tx_obj.tx_json.query.out_val,
          parser_tx_obj.tx_json.query.out_val_len, 0, &pageCount))

      ZEMU_LOGF(200, "[ZEMU] %s : %s", tmp_key,
                parser_tx_obj.tx_json.query.out_val)

      switch (root_item_idx) {
      case root_item_memo: {
        if (strlen(parser_tx_obj.tx_json.query.out_val) == 0) {
          err = parser_query_no_results;
          continue;
        }
        break;
      }
      case root_item_msgs: {
        // Note: if we are dealing with the message field, Ledger has requested
        // that we group. This means that if all messages share the same time,
        // we should only count the type field once This is indicated by
        // `parser_tx_obj.flags.msg_type_grouping`

        // GROUPING: Message Type
        if (parser_tx_obj.tx_json.flags.msg_type_grouping &&
            is_msg_type_field(tmp_key)) {
          // First message, initialize expected type
          if (parser_tx_obj.tx_json.filter_msg_type_count == 0) {

            if (strlen(tmp_val) >= sizeof(reference_msg_type)) {
              return parser_unexpected_type;
            }

            snprintf(reference_msg_type, sizeof(reference_msg_type), "%s",
                     tmp_val);
            parser_tx_obj.tx_json.filter_msg_type_valid_idx = current_item_idx;
          }

          if (strcmp(reference_msg_type, tmp_val) != 0) {
            // different values, so disable grouping
            parser_tx_obj.tx_json.flags.msg_type_grouping = 0;
            parser_tx_obj.tx_json.filter_msg_type_count = 0;
          }

          parser_tx_obj.tx_json.filter_msg_type_count++;
        }

        // GROUPING: Message From
        if (parser_tx_obj.tx_json.flags.msg_from_grouping &&
            is_msg_from_field(tmp_key)) {
          // First message, initialize expected from
          if (parser_tx_obj.tx_json.filter_msg_from_count == 0) {
            snprintf(reference_msg_from, sizeof(reference_msg_from), "%s",
                     tmp_val);
            parser_tx_obj.tx_json.filter_msg_from_valid_idx = current_item_idx;
          }

          if (strcmp(reference_msg_from, tmp_val) != 0) {
            // different values, so disable grouping
            parser_tx_obj.tx_json.flags.msg_from_grouping = 0;
            parser_tx_obj.tx_json.filter_msg_from_count = 0;
          }

          parser_tx_obj.tx_json.filter_msg_from_count++;
        }

        ZEMU_LOGF(200, "[ZEMU] %s [%d/%d]", tmp_key,
                  parser_tx_obj.tx_json.filter_msg_type_count,
                  parser_tx_obj.tx_json.filter_msg_from_count);
        break;
      }
      default:
        break;
      }

      display_cache.root_item_number_subitems[root_item_idx]++;
      current_item_idx++;
    }

    if (err != parser_query_no_results && err != parser_no_data) {
      return err;
    }

    display_cache.total_item_count +=
        display_cache.root_item_number_subitems[root_item_idx];
  }

  parser_tx_obj.tx_json.flags.cache_valid = 1;

  CHECK_PARSER_ERR(calculate_is_default_chainid())

  // turn off grouping if we are not in expert mode
  bool is_expert_or_default = false;
  CHECK_PARSER_ERR(
      tx_is_expert_mode_or_not_default_chainid(&is_expert_or_default))
  if (is_expert_or_default) {
    parser_tx_obj.tx_json.flags.msg_from_grouping = 0;
  }

  // check if from reference value matches the device address that will be
  // signing
  parser_tx_obj.tx_json.flags.msg_from_grouping_hide_all = 0;
  if (address_matches_own(reference_msg_from)) {
    parser_tx_obj.tx_json.flags.msg_from_grouping_hide_all = 1;
  }

  return parser_ok;
}

__Z_INLINE parser_error_t is_default_chainid(bool *is_default) {
  if (is_default == NULL) {
    return parser_unexpected_value;
  }

  CHECK_PARSER_ERR(tx_indexRootFields())
  *is_default = display_cache.is_default_chain;

  return parser_ok;
}

parser_error_t
tx_is_expert_mode_or_not_default_chainid(bool *expert_or_default) {
  if (expert_or_default == NULL) {
    return parser_unexpected_value;
  }

  bool is_default = false;
  CHECK_PARSER_ERR(is_default_chainid(&is_default))
  *expert_or_default = app_mode_expert() || !is_default;

  return parser_ok;
}

__Z_INLINE parser_error_t get_subitem_count(root_item_e root_item,
                                            uint8_t *num_items) {
  if (num_items == NULL) {
    return parser_unexpected_value;
  }

  CHECK_PARSER_ERR(tx_indexRootFields())
  if (display_cache.total_item_count == 0) {
    *num_items = 0;
    return parser_ok;
  }

  int32_t tmp_num_items = display_cache.root_item_number_subitems[root_item];
  bool is_expert_or_default = false;

  switch (root_item) {
  case root_item_chain_id:
  case root_item_sequence:
  case root_item_account_number:
    CHECK_PARSER_ERR(
        tx_is_expert_mode_or_not_default_chainid(&is_expert_or_default))
    if (!is_expert_or_default) {
      tmp_num_items = 0;
    }
    break;
  case root_item_msgs: {
    // Remove grouped items from list
    if (parser_tx_obj.tx_json.flags.msg_type_grouping &&
        parser_tx_obj.tx_json.filter_msg_type_count > 0) {
      tmp_num_items += 1; // we leave main type
      tmp_num_items -= parser_tx_obj.tx_json.filter_msg_type_count;
    }
    if (parser_tx_obj.tx_json.flags.msg_from_grouping &&
        parser_tx_obj.tx_json.filter_msg_from_count > 0) {
      if (!parser_tx_obj.tx_json.flags.msg_from_grouping_hide_all) {
        tmp_num_items += 1; // we leave main from
      }
      tmp_num_items -= parser_tx_obj.tx_json.filter_msg_from_count;
    }
    break;
  }
  case root_item_memo:
    break;
  case root_item_fee:
    CHECK_PARSER_ERR(
        tx_is_expert_mode_or_not_default_chainid(&is_expert_or_default))
    if (!is_expert_or_default) {
      tmp_num_items = 1; // Only Amount
    }
    break;
  case root_item_tip:
    tmp_num_items += 0;
    break;
  default:
    break;
  }

  // Validate bounds before casting to uint8_t
  if (tmp_num_items < 0 || tmp_num_items > UINT8_MAX) {
    return parser_unexpected_number_items;
  }

  *num_items = (uint8_t)tmp_num_items;

  return parser_ok;
}

__Z_INLINE parser_error_t retrieve_tree_indexes(uint8_t display_index,
                                                root_item_e *root_item,
                                                uint8_t *subitem_index) {
  if (root_item == NULL || subitem_index == NULL) {
    return parser_unexpected_value;
  }

  // Find root index | display_index idx -> item_index
  // consume indexed subpages until we get the item index in the subpage
  *root_item = 0;
  *subitem_index = 0;
  uint8_t num_items;

  CHECK_PARSER_ERR(get_subitem_count(*root_item, &num_items));
  while (num_items == 0) {
    (*root_item)++;
    CHECK_PARSER_ERR(get_subitem_count(*root_item, &num_items));
  }

  for (uint16_t i = 0; i < display_index; i++) {
    (*subitem_index)++;
    uint8_t subitem_count = 0;
    CHECK_PARSER_ERR(get_subitem_count(*root_item, &subitem_count));
    if (*subitem_index >= subitem_count) {
      // Advance root index and skip empty items
      *subitem_index = 0;
      (*root_item)++;

      uint8_t num_items_2 = 0;
      CHECK_PARSER_ERR(get_subitem_count(*root_item, &num_items_2));
      while (num_items_2 == 0) {
        (*root_item)++;
        CHECK_PARSER_ERR(get_subitem_count(*root_item, &num_items_2));
      }
    }
  }

  if (*root_item > NUM_REQUIRED_ROOT_PAGES) {
    return parser_no_data;
  }

  return parser_ok;
}

parser_error_t tx_display_numItems(uint8_t *num_items) {
  *num_items = 0;
  CHECK_PARSER_ERR(tx_indexRootFields())

  uint16_t total = 0;
  uint8_t n_items = 0;
  for (root_item_e root_item = 0; root_item < NUM_REQUIRED_ROOT_PAGES;
       root_item++) {
    CHECK_PARSER_ERR(get_subitem_count(root_item, &n_items))
    total += n_items;
  }

  // Reject transactions with too many items to display safely
  if (total > UINT8_MAX) {
    return parser_unexpected_number_items;
  }

  *num_items = (uint8_t)total;
  return parser_ok;
}

// This function assumes that the tx_ctx has been set properly
parser_error_t tx_display_query(uint16_t displayIdx, char *outKey,
                                uint16_t outKeyLen,
                                uint16_t *ret_value_token_index) {
  CHECK_PARSER_ERR(tx_indexRootFields())

  uint8_t num_items;
  CHECK_PARSER_ERR(tx_display_numItems(&num_items))

  if (displayIdx >= num_items) {
    return parser_display_idx_out_of_range;
  }

  root_item_e root_index = 0;
  uint8_t subitem_index = 0;
  CHECK_PARSER_ERR(
      retrieve_tree_indexes(displayIdx, &root_index, &subitem_index))

  // Prepare query
  static char tmp_val[2];
  INIT_QUERY_CONTEXT(outKey, outKeyLen, tmp_val, sizeof(tmp_val), 0,
                     get_root_max_level(root_index))
  parser_tx_obj.tx_json.query.item_index = subitem_index;
  parser_tx_obj.tx_json.query._item_index_current = 0;

  strncpy_s(outKey, get_required_root_item(root_index), outKeyLen);

  if (!display_cache.root_item_start_token_valid[root_index]) {
    return parser_no_data;
  }

  CHECK_PARSER_ERR(
      tx_traverse_find(display_cache.root_item_start_token_idx[root_index],
                       ret_value_token_index))

  return parser_ok;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

static const key_subst_t key_substitutions[] = {
    {"chain_id", "Chain ID"},
    {"account_number", "Account"},
    {"sequence", "Sequence"},
    {"memo", "Memo"},
    {"fee/amount", "Fee"},
    {"fee/gas", "Gas"},
    {"fee/gas_limit", "Gas Limit"},
    {"fee/granter", "Granter"},
    {"fee/payer", "Payer"},
    {"msgs/type", "Type"},

    {"tip/amount", "Tip"},
    {"tip/tipper", "Tipper"},

    {"msgs/inputs/address", "Source Address"},
    {"msgs/inputs/coins", "Source Coins"},
    {"msgs/outputs/address", "Dest Address"},
    {"msgs/outputs/coins", "Dest Coins"},

    {"msgs/value/inputs/address", "Source Address"},
    {"msgs/value/inputs/coins", "Source Coins"},
    {"msgs/value/outputs/address", "Dest Address"},
    {"msgs/value/outputs/coins", "Dest Coins"},

    {"msgs/value/from_address", "From"},
    {"msgs/value/to_address", "To"},
    {"msgs/value/amount", "Amount"},
    {"msgs/value/delegator_address", "Delegator"},
    {"msgs/value/validator_address", "Validator"},
    {"msgs/value/withdraw_address", "Withdraw Address"},
    {"msgs/value/validator_src_address", "Validator Source"},
    {"msgs/value/validator_dst_address", "Validator Dest"},
    {"msgs/value/description", "Description"},
    {"msgs/value/initial_deposit/amount", "Deposit Amount"},
    {"msgs/value/initial_deposit/denom", "Deposit Denom"},
    {"msgs/value/proposal_type", "Proposal"},
    {"msgs/value/proposer", "Proposer"},
    {"msgs/value/title", "Title"},
    {"msgs/value/depositor", "Sender"},
    {"msgs/value/proposal_id", "Proposal ID"},
    {"msgs/value/voter", "Description"},
    {"msgs/value/option", "Option"},
};

parser_error_t tx_display_make_friendly() {
  CHECK_PARSER_ERR(tx_indexRootFields())

  // post process keys
  for (size_t i = 0; i < array_length(key_substitutions); i++) {
    const char *str1 = (const char *)PIC(key_substitutions[i].str1);
    const char *str2 = (const char *)PIC(key_substitutions[i].str2);
    const uint16_t str1Len = strlen(str1);
    const uint16_t str2Len = strlen(str2);

    const uint16_t outKeyLen = strnlen(parser_tx_obj.tx_json.query.out_key,
                                       parser_tx_obj.tx_json.query.out_key_len);
    if ((outKeyLen == str1Len &&
         strncmp(parser_tx_obj.tx_json.query.out_key, str1, str1Len) == 0) &&
        parser_tx_obj.tx_json.query.out_key_len >= str2Len) {
      MEMZERO(parser_tx_obj.tx_json.query.out_key,
              parser_tx_obj.tx_json.query.out_key_len);
      MEMCPY(parser_tx_obj.tx_json.query.out_key, str2, str2Len);
      break;
    }
  }

  return parser_ok;
}

static const ascii_subst_t ascii_substitutions[] = {
    {0x07, 'a'}, {0x08, 'b'}, {0x0C, 'f'}, {0x0A, 'n'},
    {0x0D, 'r'}, {0x09, 't'}, {0x0B, 'v'}, {0x5C, '\\'},
};

parser_error_t tx_display_translation(char *dst, uint16_t dstLen, char *src,
                                      uint16_t srcLen) {
  if (dst == NULL || src == NULL) {
    return parser_unexpected_value;
  }

  MEMZERO(dst, dstLen);
  char *p = src;
  uint16_t count = 0;

  while (p < src + srcLen) {
    utf8_int32_t tmp_codepoint = 0;
    p = utf8codepoint(p, &tmp_codepoint);

    if (tmp_codepoint < 0x0F || tmp_codepoint == 0x5C) {
      bool found = false;
      for (size_t i = 0; i < array_length(ascii_substitutions); i++) {
        if ((char)tmp_codepoint == ascii_substitutions[i].ascii_code) {
          ASSERT_PTR_BOUNDS(count, dstLen);
          *dst++ = '\\';
          ASSERT_PTR_BOUNDS(count, dstLen);
          *dst++ = ascii_substitutions[i].str;
          found = true;
          break;
        }
      }
      if (!found) {
        // Write out the value as a hex escape, \xNN
        // Check for snprintf buffer size (includes null terminator)
        if (count + HEX_ESCAPE_SNPRINTF_SIZE > dstLen) {
          return parser_transaction_too_big;
        }
        snprintf(dst, HEX_ESCAPE_SNPRINTF_SIZE, "\\x%.02X", tmp_codepoint);
        dst += HEX_ESCAPE_LEN;
        count += HEX_ESCAPE_LEN;
      }
    } else if (tmp_codepoint >= 32 && tmp_codepoint <= ((int32_t)0x7F)) {
      ASSERT_PTR_BOUNDS(count, dstLen);
      *dst++ = (char)tmp_codepoint;
    } else {
      ASSERT_PTR_BOUNDS(count, dstLen);
      *dst++ = '\\';

      uint8_t bytes_to_print = 8;
      int32_t swapped = ZX_SWAP(tmp_codepoint);
      if (tmp_codepoint > 0xFFFF) {
        ASSERT_PTR_BOUNDS(count, dstLen);
        *dst++ = 'U';
      } else {
        ASSERT_PTR_BOUNDS(count, dstLen);
        *dst++ = 'u';
        bytes_to_print = 4;
        swapped = (swapped >> 16) & 0xFFFF;
      }

      if (dstLen < (bytes_to_print + count)) {
        return parser_unexpected_value;
      }

      char buf[18] = {0};
      array_to_hexstr(buf, sizeof(buf), (uint8_t *)&swapped, 4);
      for (int i = 0; i < bytes_to_print; i++) {
        ASSERT_PTR_BOUNDS(count, dstLen);
        *dst++ = (buf[i] >= 'a' && buf[i] <= 'z') ? (buf[i] - 32) : buf[i];
      }
    }
  }

  if (src[srcLen - 1] == ' ' || src[srcLen - 1] == '@') {
    if (count >= dstLen) {
      return parser_unexpected_value;
    }
    ASSERT_PTR_BOUNDS(count, dstLen);
    *dst++ = '@';
  }

  // Terminate string
  ASSERT_PTR_BOUNDS(count, dstLen);
  *dst = 0;
  return parser_ok;
}

#ifdef __cplusplus
#pragma clang diagnostic pop
#endif
