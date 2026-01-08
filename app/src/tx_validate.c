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

  if (strcmp(first, second) <= 0) {
    return 1;
  }

  return 0;
}

int8_t dictionaries_sorted(parsed_json_t *json) {
  if (json == NULL) {
    return 0;
  }

  for (uint32_t i = 0; i < json->numberOfTokens; i++) {
    if (json->tokens[i].type == JSMN_OBJECT) {

      uint16_t count;

      if (object_get_element_count(json, i, &count) != parser_ok) {
        return 0;
      }

      if (count > 1) {
        uint16_t prev_token_index;
        if (object_get_nth_key(json, i, 0, &prev_token_index) != parser_ok) {
          return 0;
        }

        for (int j = 1; j < count; j++) {
          uint16_t next_token_index;

          if (object_get_nth_key(json, i, j, &next_token_index) != parser_ok) {
            return 0;
          }

          if (!is_sorted(prev_token_index, next_token_index, json)) {
            return 0;
          }
          prev_token_index = next_token_index;
        }
      }
    }
  }
  return 1;
}

parser_error_t tx_validate(parsed_json_t *json) {
  parser_error_t err = contains_whitespace(json);
  if (err != parser_ok) {
    return err;
  }

  if (dictionaries_sorted(json) != 1) {
    return parser_json_is_not_sorted;
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

  err = object_get_value(json, 0, "msgs", &token_index);
  if (err != parser_ok)
    return parser_json_missing_msgs;

  err = object_get_value(json, 0, "account_number", &token_index);
  if (err != parser_ok)
    return parser_json_missing_account_number;

  err = object_get_value(json, 0, "memo", &token_index);
  if (err != parser_ok)
    return parser_json_missing_memo;

  return parser_ok;
}
