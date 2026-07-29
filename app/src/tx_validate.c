/*******************************************************************************
 *   (c) 2018, 2019 Zondax GmbH
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

#include "json/json_parser.h"
#include <common/parser_common.h>
#include <jsmn.h>
#include <zxmacros.h>

const char whitespaces[] = {
    0x20, // space ' '
    0x0c, // form_feed '\f'
    0x0a, // line_feed, '\n'
    0x0d, // carriage_return, '\r'
    0x09, // horizontal_tab, '\t'
    0x0b  // vertical_tab, '\v'
};

int8_t is_space(char c) {
  for (uint32_t i = 0; i < sizeof(whitespaces); i++) {
    if (whitespaces[i] == c) {
      return 1;
    }
  }
  return 0;
}

// contains_whitespace only inspects the gaps between tokens, so a control byte
// inside a value is invisible to it. Those bytes are rendered verbatim by the
// JSON display path, and each consumes a whole line of a review page while
// costing one character of the budget the page size is derived from, so later
// bytes can be signed without ever reaching the screen. The canonical compact
// encoding never emits one.
//
// Only string and primitive spans are scanned: an object or array token spans
// its whole subtree, including the gaps that contains_whitespace owns.
static parser_error_t contains_control_chars(parsed_json_t *json) {
  if (json == NULL) {
    return parser_unexpected_value;
  }

  for (uint32_t i = 0; i < json->numberOfTokens; i++) {
    const jsmntok_t *token = &json->tokens[i];

    if (token->type == JSMN_UNDEFINED) {
      break;
    }
    if (token->type != JSMN_STRING && token->type != JSMN_PRIMITIVE) {
      continue;
    }
    if (token->start < 0 || token->end < token->start ||
        token->end > (int)json->bufferLen) {
      return parser_unexpected_value;
    }

    for (int j = token->start; j < token->end; j++) {
      if ((uint8_t)json->buffer[j] < 0x20) {
        return parser_unexpected_characters;
      }
    }
  }

  return parser_ok;
}

parser_error_t contains_whitespace(parsed_json_t *json) {
  if (json == NULL) {
    return parser_unexpected_value;
  }

  // Validate we have at least one token
  if (json->numberOfTokens == 0) {
    return parser_unexpected_value;
  }

  int start = 0;
  const int last_element_index = json->tokens[0].end;

  // Validate last_element_index is within buffer bounds
  if (last_element_index < 0 || last_element_index > (int)json->bufferLen) {
    return parser_unexpected_value;
  }

  // Starting at token 1 because token 0 contains full tx
  for (uint32_t i = 1; i < json->numberOfTokens; i++) {
    if (json->tokens[i].type != JSMN_UNDEFINED) {
      const int end = json->tokens[i].start;
      // Bounds check: ensure end is within buffer
      for (int j = start; j < end && j < (int)json->bufferLen; j++) {
        if (is_space(json->buffer[j]) == 1) {
          return parser_json_contains_whitespace;
        }
      }
      start = json->tokens[i].end + 1;
      if (start < 0) {
        return parser_json_unexpected_error;
      }
    } else {
      return parser_ok;
    }
  }

  // Bounds check: ensure start is within buffer
  while (start < last_element_index && start < (int)json->bufferLen &&
         json->buffer[start] != '\0') {
    if (is_space(json->buffer[start])) {
      return parser_json_contains_whitespace;
    }
    start++;
  }
  return parser_ok;
}

int8_t is_sorted(uint16_t first_index, uint16_t second_index,
                 parsed_json_t *json) {
  if (json == NULL) {
    return 0;
  }

  // Validate token indices are within bounds
  if (first_index >= json->numberOfTokens ||
      second_index >= json->numberOfTokens) {
    return 0;
  }

  char first[256];
  char second[256];
  MEMZERO(first, sizeof first);
  MEMZERO(second, sizeof second);

  // Validate first token buffer bounds
  if (json->tokens[first_index].start < 0 ||
      json->tokens[first_index].end < 0 ||
      json->tokens[first_index].end > (int)json->bufferLen ||
      json->tokens[first_index].start > json->tokens[first_index].end) {
    return 0;
  }

  size_t size = json->tokens[first_index].end - json->tokens[first_index].start;
  if (size >= sizeof(first)) {
    return 0;
  }

  strncpy(first, json->buffer + json->tokens[first_index].start, size);
  first[size] = '\0';

  // Validate second token buffer bounds
  if (json->tokens[second_index].start < 0 ||
      json->tokens[second_index].end < 0 ||
      json->tokens[second_index].end > (int)json->bufferLen ||
      json->tokens[second_index].start > json->tokens[second_index].end) {
    return 0;
  }

  size = json->tokens[second_index].end - json->tokens[second_index].start;
  if (size >= sizeof(second))
    return 0;

  strncpy(second, json->buffer + json->tokens[second_index].start, size);
  second[size] = '\0';

  int cmp = strcmp(first, second);
  if (cmp < 0) {
    return 1;
  }
  if (cmp == 0) {
    return -1;
  }

  return 0;
}

parser_error_t dictionaries_sorted(parsed_json_t *json) {
  if (json == NULL) {
    return parser_unexpected_value;
  }

  for (uint32_t i = 0; i < json->numberOfTokens; i++) {
    if (json->tokens[i].type == JSMN_OBJECT) {

      uint16_t count;

      if (object_get_element_count(json, i, &count) != parser_ok) {
        return parser_unexpected_value;
      }

      if (count > 1) {
        uint16_t prev_token_index;
        if (object_get_nth_key(json, i, 0, &prev_token_index) != parser_ok) {
          return parser_unexpected_value;
        }

        for (int j = 1; j < count; j++) {
          uint16_t next_token_index;

          if (object_get_nth_key(json, i, j, &next_token_index) != parser_ok) {
            return parser_unexpected_value;
          }

          int8_t order = is_sorted(prev_token_index, next_token_index, json);
          if (order == -1) {
            return parser_duplicated_field;
          }
          if (order != 1) {
            return parser_json_is_not_sorted;
          }
          prev_token_index = next_token_index;
        }
      }
    }
  }
  return parser_ok;
}

// Closed-world key check: every key directly under the object at
// object_token_index must appear in allowed_keys. Unknown keys would otherwise
// be part of the signed bytes without ever being shown to the user.
static parser_error_t validate_allowed_keys(parsed_json_t *json,
                                            uint16_t object_token_index,
                                            const char *const *allowed_keys,
                                            uint16_t allowed_count) {
  uint16_t key_count = 0;
  parser_error_t err =
      object_get_element_count(json, object_token_index, &key_count);
  if (err != parser_ok) {
    return err;
  }

  for (uint16_t i = 0; i < key_count; i++) {
    uint16_t key_token_idx = 0;
    err = object_get_nth_key(json, object_token_index, i, &key_token_idx);
    if (err != parser_ok) {
      return err;
    }

    if (key_token_idx >= json->numberOfTokens) {
      return parser_unexpected_field;
    }

    int start = json->tokens[key_token_idx].start;
    int end = json->tokens[key_token_idx].end;
    if (start < 0 || end < start || end > (int)json->bufferLen) {
      return parser_unexpected_field;
    }
    int key_len = end - start;
    const char *key_ptr = json->buffer + start;

    bool found = false;
    for (uint16_t j = 0; j < allowed_count; j++) {
      // allowed_keys[j] points to a string literal whose address is fixed at
      // link time; on a relocated Ledger app it must go through PIC() before it
      // is dereferenced, otherwise strlen/MEMCMP read a wild pointer and the
      // app crashes (SIGSEGV). PIC() is a no-op on the host build.
      const char *allowed_key = (const char *)PIC(allowed_keys[j]);
      size_t allowed_len = strlen(allowed_key);
      if ((int)allowed_len == key_len &&
          MEMCMP(allowed_key, key_ptr, key_len) == 0) {
        found = true;
        break;
      }
    }
    if (!found) {
      return parser_unexpected_field;
    }
  }

  return parser_ok;
}

parser_error_t tx_validate(parsed_json_t *json) {
  if (json == NULL) {
    return parser_unexpected_value;
  }

  // When the top-level token is an object, require it to consume the entire
  // input so hidden bytes after the parsed root cannot be signed unseen.
  // Non-object roots fall through to the structural checks below, which
  // report a clearer error (missing chain_id etc.).
  if (json->numberOfTokens > 0 && json->tokens[0].type == JSMN_OBJECT &&
      (json->tokens[0].start != 0 ||
       json->tokens[0].end != (int)json->bufferLen)) {
    return parser_unexpected_characters;
  }

  parser_error_t err = contains_control_chars(json);
  if (err != parser_ok) {
    return err;
  }

  err = contains_whitespace(json);
  if (err != parser_ok) {
    return err;
  }

  err = dictionaries_sorted(json);
  if (err != parser_ok) {
    return err;
  }

  uint16_t token_index;

  err = object_get_value(json, 0, "chain_id", &token_index);
  if (err != parser_ok)
    return parser_json_missing_chain_id;

  err = object_get_value(json, 0, "sequence", &token_index);
  if (err != parser_ok)
    return parser_json_missing_sequence;

  err = object_get_value(json, 0, "fee", &token_index);
  if (err != parser_ok)
    return parser_json_missing_fee;
  uint16_t fee_token_index = token_index;

  err = object_get_value(json, 0, "msgs", &token_index);
  if (err != parser_ok)
    return parser_json_missing_msgs;

  err = object_get_value(json, 0, "account_number", &token_index);
  if (err != parser_ok)
    return parser_json_missing_account_number;

  err = object_get_value(json, 0, "memo", &token_index);
  if (err != parser_ok)
    return parser_json_missing_memo;

  // Closed-world allowlist: anything not explicitly allowed at the SignDoc
  // root or inside StdFee is rejected so future host-side fields cannot be
  // signed without the device knowing how to display them.
  static const char *const allowed_root_keys[] = {
      "account_number", "chain_id", "fee", "memo",
      "msgs",           "sequence", "tip", "timeout_height",
  };
  err = validate_allowed_keys(json, 0, allowed_root_keys,
                              sizeof(allowed_root_keys) /
                                  sizeof(allowed_root_keys[0]));
  if (err != parser_ok) {
    return err;
  }

  static const char *const allowed_fee_keys[] = {
      "amount",
      "gas",
      "granter",
      "payer",
  };
  err = validate_allowed_keys(json, fee_token_index, allowed_fee_keys,
                              sizeof(allowed_fee_keys) /
                                  sizeof(allowed_fee_keys[0]));
  if (err != parser_ok) {
    return err;
  }

  return parser_ok;
}
