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

// A signed document carries JSON escapes; the chain sees what they decode to.
// The review used to page the raw span, so "cosmos1abcAdef" was read out
// with a literal backslash-u while the chain executed "cosmos1abcAdef", and
// "SAFE\rDRAINED" passed the control-byte check -- whose subject is the raw
// bytes, all printable here -- while decoding to a carriage return.
//
// Decode while paging so the screen shows the value the chain will act on.
// Decoding never grows a string (\uXXXX is six bytes in and at most four out),
// so a page still fits the buffer it always did, and the value stays paged in
// decoded bytes exactly as it was paged in raw ones.
//
// Anything a decoder could disagree about is refused rather than guessed: an
// unknown escape, a truncated one, a lone surrogate, or a decoded byte below
// 0x20 -- the same bound the raw check applies, now reaching the escaped form
// that used to slip under it.
//
// Decoding is only worth doing while it produces a character the screen can
// show. Above printable ASCII it does not: U+2028 and U+2029 draw as nothing,
// U+200B and U+FEFF as nothing, U+202E as a change of direction, the C1 range
// as nothing at all -- and U+2028/U+2029 are exactly the code points Go's
// encoder escapes, so they arrive this way in ordinary documents. There the
// escape is copied through as it was written, which is what the review drew
// before it decoded anything. Raw non-ASCII bytes are a separate, older gap
// (see the look-alike case in tests/control_bytes.cpp) and are untouched here.

__Z_INLINE parser_error_t hex_nibble(char c, uint8_t *out) {
  if (c >= '0' && c <= '9') {
    *out = (uint8_t)(c - '0');
  } else if (c >= 'a' && c <= 'f') {
    *out = (uint8_t)(c - 'a' + 10);
  } else if (c >= 'A' && c <= 'F') {
    *out = (uint8_t)(c - 'A' + 10);
  } else {
    return parser_unexpected_characters;
  }
  return parser_ok;
}

__Z_INLINE parser_error_t read_u16_escape(const char *in, uint16_t inLen,
                                          uint16_t at, uint16_t *codeUnit) {
  // at points just past the "u"; four hex digits have to follow.
  if ((uint32_t)at + 4 > inLen) {
    return parser_unexpected_characters;
  }
  uint16_t value = 0;
  for (uint8_t i = 0; i < 4; i++) {
    uint8_t nibble = 0;
    CHECK_PARSER_ERR(hex_nibble(in[at + i], &nibble))
    value = (uint16_t)((value << 4) | nibble);
  }
  *codeUnit = value;
  return parser_ok;
}

// Decodes one element of the string. Reports how many input bytes it consumed
// and writes what the screen should show into out/outLen. Never more bytes out
// than in, so a page still fits the buffer it always did: a decoded escape
// shrinks, and one copied through verbatim is the same length.
#define JSON_DECODE_MAX_OUT 12

__Z_INLINE parser_error_t json_decode_one(const char *in, uint16_t inLen,
                                          uint16_t at, uint16_t *consumed,
                                          uint8_t *out, uint8_t *outLen) {
  const uint8_t c = (uint8_t)in[at];

  if (c < 0x20) {
    return parser_unexpected_characters;
  }

  if (c != '\\') {
    out[0] = c;
    *outLen = 1;
    *consumed = 1;
    return parser_ok;
  }

  if ((uint32_t)at + 1 >= inLen) {
    return parser_unexpected_characters;
  }

  const char esc = in[at + 1];
  switch (esc) {
  case '"':
  case '\\':
  case '/':
    out[0] = (uint8_t)esc;
    *outLen = 1;
    *consumed = 2;
    return parser_ok;
  // \b \f \n \r \t all decode below 0x20, which the document is not allowed
  // to contain however it is spelled.
  case 'b':
  case 'f':
  case 'n':
  case 'r':
  case 't':
    return parser_unexpected_characters;
  case 'u':
    break;
  default:
    return parser_unexpected_characters;
  }

  uint16_t unit = 0;
  CHECK_PARSER_ERR(read_u16_escape(in, inLen, (uint16_t)(at + 2), &unit))
  uint16_t used = 6;
  uint32_t cp = unit;

  if (unit >= 0xD800 && unit <= 0xDBFF) {
    // High surrogate: a low surrogate must follow, spelled as its own escape.
    if ((uint32_t)at + 8 > inLen || in[at + 6] != '\\' || in[at + 7] != 'u') {
      return parser_unexpected_characters;
    }
    uint16_t low = 0;
    CHECK_PARSER_ERR(read_u16_escape(in, inLen, (uint16_t)(at + 8), &low))
    if (low < 0xDC00 || low > 0xDFFF) {
      return parser_unexpected_characters;
    }
    cp = 0x10000u + (((uint32_t)(unit - 0xD800u)) << 10) + (low - 0xDC00u);
    used = 12;
  } else if (unit >= 0xDC00 && unit <= 0xDFFF) {
    // Low surrogate with no high surrogate before it.
    return parser_unexpected_characters;
  }

  if (cp < 0x20) {
    return parser_unexpected_characters;
  }

  if (cp > 0x7E) {
    // Nothing legible to decode to: copy the escape through exactly as the
    // document spells it, so the screen names the code point instead of
    // drawing an invisible one. `used` is 6 or 12, both within
    // JSON_DECODE_MAX_OUT, and equal to what was consumed.
    for (uint16_t i = 0; i < used; i++) {
      out[i] = (uint8_t)in[at + i];
    }
    *outLen = (uint8_t)used;
    *consumed = used;
    return parser_ok;
  }

  out[0] = (uint8_t)cp;
  *outLen = 1;
  *consumed = used;
  return parser_ok;
}

// pageStringExt for a value that is still in its JSON escaped form: same
// pagination, over the decoded bytes.
__Z_INLINE parser_error_t page_json_string(char *outValue, uint16_t outValueLen,
                                           const char *inValue, uint16_t inLen,
                                           uint8_t pageIdx,
                                           uint8_t *pageCount) {
  MEMZERO(outValue, outValueLen);
  *pageCount = 0;

  if (outValueLen == 0) {
    return parser_unexpected_buffer_end;
  }
  const uint16_t chunkLen = (uint16_t)(outValueLen - 1);
  if (chunkLen == 0) {
    return parser_unexpected_buffer_end;
  }
  if (inLen == 0) {
    *pageCount = 1;
    return parser_ok;
  }

  uint8_t out[JSON_DECODE_MAX_OUT] = {0};
  uint8_t outLen = 0;
  uint16_t consumed = 0;

  // First pass: the displayed length, which is what the value is paged by.
  uint32_t decodedLen = 0;
  for (uint16_t at = 0; at < inLen;) {
    CHECK_PARSER_ERR(
        json_decode_one(inValue, inLen, at, &consumed, out, &outLen))
    decodedLen += outLen;
    at = (uint16_t)(at + consumed);
  }

  const uint32_t totalPages =
      (decodedLen + chunkLen - 1) / chunkLen + (decodedLen == 0 ? 1 : 0);
  if (totalPages > UINT8_MAX) {
    // Same refusal pageStringExt makes: a value that cannot be paged whole is
    // not shown at all.
    return parser_unexpected_value;
  }
  *pageCount = (uint8_t)totalPages;

  if (pageIdx >= *pageCount) {
    return parser_ok;
  }

  // Second pass: skip the pages before this one, then fill this one.
  const uint32_t skip = (uint32_t)pageIdx * chunkLen;
  uint32_t seen = 0;
  uint16_t written = 0;
  for (uint16_t at = 0; at < inLen && written < chunkLen;) {
    CHECK_PARSER_ERR(
        json_decode_one(inValue, inLen, at, &consumed, out, &outLen))
    at = (uint16_t)(at + consumed);
    for (uint8_t i = 0; i < outLen && written < chunkLen; i++) {
      if (seen >= skip) {
        outValue[written++] = (char)out[i];
      }
      seen++;
    }
  }
  outValue[written] = '\0';
  return parser_ok;
}

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
    CHECK_PARSER_ERR(page_json_string(out_val, out_val_len, inValue, inLen,
                                      pageIdx, pageCount))
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

// The choice tx_display takes once per document (see tx_msg_itemize_exec), read
// back here by both walks so the item count and the rendering agree.
//
// Held as the fallback rather than as the choice because a Ledger app links
// with an empty .data section: zero-initialised has to mean the ordinary case,
// which is that the executed messages get their own screens.
static bool exec_blob_fallback;
static bool saw_exec;

void tx_msg_itemize_exec(bool enabled) {
  exec_blob_fallback = !enabled;
  saw_exec = false;
}

bool tx_msg_saw_exec(void) { return saw_exec; }

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
  if (msg_type_equals(type_str, type_len, "cosmos-sdk/MsgExec")) {
    saw_exec = true;
    return exec_blob_fallback ? MSG_BASE_FLATTEN_LEVEL : MSG_EXEC_FLATTEN_LEVEL;
  }
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

uint8_t tx_msg_max_depth(uint16_t msg_token_index) {
  // Same dispatch as tx_msg_max_level, on the same "type" field. A message that
  // carries other messages is the only shape that needs a deeper budget, and it
  // gets it only while its own subtree is being walked -- every other document
  // recurses exactly as far as it did before.
  uint16_t type_token_index = 0;
  if (object_get_value(&parser_tx_obj.tx_json.json, msg_token_index, "type",
                       &type_token_index) != parser_ok) {
    return MAX_RECURSION_DEPTH;
  }

  const int16_t start =
      parser_tx_obj.tx_json.json.tokens[type_token_index].start;
  const int16_t end = parser_tx_obj.tx_json.json.tokens[type_token_index].end;
  if (start < 0 || end < start) {
    return MAX_RECURSION_DEPTH;
  }

  const char *type_str = parser_tx_obj.tx_json.tx + start;
  const size_t type_len = (size_t)(end - start);
  if (msg_type_equals(type_str, type_len, "cosmos-sdk/MsgExec")) {
    return exec_blob_fallback ? MAX_RECURSION_DEPTH : MSG_EXEC_RECURSION_DEPTH;
  }

  return MAX_RECURSION_DEPTH;
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

  // Count with the helper that matches the token. object_get_element_count
  // walks key/value pairs, so on an array it returns half the elements
  // (rounded up): the walk below then never reached the second half of an
  // array of strings, and those values were missing from the review.
  if (token_type == JSMN_ARRAY) {
    CHECK_PARSER_ERR(array_get_element_count(&parser_tx_obj.tx_json.json,
                                             root_token_index, &el_count))
  } else {
    CHECK_PARSER_ERR(object_get_element_count(&parser_tx_obj.tx_json.json,
                                              root_token_index, &el_count))
  }

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
    const uint8_t saved_max_depth = parser_tx_obj.tx_json.query.max_depth;

    for (uint16_t i = 0; i < el_count; ++i) {
      uint16_t element_index;
      CHECK_PARSER_ERR(array_get_nth_element(
          &parser_tx_obj.tx_json.json, root_token_index, i, &element_index))
      CHECK_APP_CANARY()

      if (is_msgs_array) {
        parser_tx_obj.tx_json.query.max_level = tx_msg_max_level(element_index);
        // Raise the budget only for the message that needs it, never for the
        // document as a whole: a deeper global limit would put every other
        // transaction two frames further down the stack for nothing.
        const uint8_t msg_depth = tx_msg_max_depth(element_index);
        if (msg_depth > parser_tx_obj.tx_json.query.max_depth) {
          parser_tx_obj.tx_json.query.max_depth = msg_depth;
        }
      }

      // When iterating along an array,
      // the level does not change but we need to count the recursion
      parser_tx_obj.tx_json.query.max_depth--;
      err = tx_traverse_find(element_index, ret_value_token_index);
      parser_tx_obj.tx_json.query.max_depth++;

      if (is_msgs_array) {
        parser_tx_obj.tx_json.query.max_level = saved_max_level;
        parser_tx_obj.tx_json.query.max_depth = saved_max_depth;
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
