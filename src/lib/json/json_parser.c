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

#include <jsmn.h>
#include <zxmacros.h>
#include <parser_common.h>
#include "json_parser.h"

#if defined(TARGET_NANOS) || defined(TARGET_NANOX)
#include "os.h"
#define EQUALS(_P, _Q, _LEN) (os_memcmp( PIC(_P), PIC(_Q), (_LEN))==0)
#else
#define EQUALS(_P, _Q, _LEN) (memcmp( (_P), (_Q), (_LEN))==0)
#endif

parser_error_t json_parse(parsed_json_t *parsed_json, const char *buffer) {
    return json_parse_s(parsed_json, buffer, strlen(buffer));
}

parser_error_t json_parse_s(parsed_json_t *parsed_json,
                         const char *buffer, uint16_t bufferLen) {
    jsmn_parser parser;
    jsmn_init(&parser);

    explicit_bzero(parsed_json, sizeof(parsed_json_t));
    parsed_json->buffer = buffer;
    parsed_json->bufferLen = bufferLen;

    int num_tokens = jsmn_parse(
        &parser,
        parsed_json->buffer,
        parsed_json->bufferLen,
        parsed_json->tokens,
        MAX_NUMBER_OF_TOKENS);

    switch (num_tokens) {
        case JSMN_ERROR_NOMEM:
            return parser_json_too_many_tokens;
        case JSMN_ERROR_INVAL:
            return parser_unexpected_characters;
        case JSMN_ERROR_PART:
            return parser_json_incomplete_json;
    }

    parsed_json->numberOfTokens = 0;
    parsed_json->isValid = 0;

    // Parsing error
    if (num_tokens <= 0) {
        return parser_json_zero_tokens;
    }

    // We cannot support if number of tokens exceeds the limit
    if (num_tokens > MAX_NUMBER_OF_TOKENS) {
        return parser_json_too_many_tokens;
    }

    parsed_json->numberOfTokens = num_tokens;
    parsed_json->isValid = true;

    return parser_ok;
}

uint16_t array_get_element_count(uint16_t array_token_index,
                                 const parsed_json_t *json) {
    if (array_token_index < 0 || array_token_index > json->numberOfTokens) {
        return 0;
    }

    jsmntok_t array_token = json->tokens[array_token_index];
    uint16_t token_index = array_token_index;
    uint16_t element_count = 0;
    uint16_t prev_element_end = array_token.start;
    while (true) {
        token_index++;
        if (token_index >= json->numberOfTokens) {
            break;
        }
        jsmntok_t current_token = json->tokens[token_index];
        if (current_token.start > array_token.end) {
            break;
        }
        if (current_token.start <= prev_element_end) {
            continue;
        }
        prev_element_end = current_token.end;
        element_count++;
    }

    return element_count;
}

int16_t array_get_nth_element(uint16_t array_token_index,
                              uint16_t element_index,
                              const parsed_json_t *json) {
    if (array_token_index < 0 || array_token_index > json->numberOfTokens) {
        return -1;
    }

    jsmntok_t array_token = json->tokens[array_token_index];
    uint16_t token_index = array_token_index;
    uint16_t element_count = 0;
    uint16_t prev_element_end = array_token.start;
    while (true) {
        token_index++;
        if (token_index >= json->numberOfTokens) {
            break;
        }
        jsmntok_t current_token = json->tokens[token_index];
        if (current_token.start > array_token.end) {
            break;
        }
        if (current_token.start <= prev_element_end) {
            continue;
        }
        prev_element_end = current_token.end;
        if (element_count == element_index) {
            return token_index;
        }
        element_count++;
    }

    return -1;
}

uint16_t object_get_element_count(uint16_t object_token_index,
                                  const parsed_json_t *json) {
    if (object_token_index < 0 || object_token_index > json->numberOfTokens) {
        return 0;
    }

    jsmntok_t object_token = json->tokens[object_token_index];
    uint16_t token_index = object_token_index;
    uint16_t element_count = 0;
    uint16_t prev_element_end = object_token.start;
    token_index++;
    while (true) {
        if (token_index >= json->numberOfTokens) {
            break;
        }
        jsmntok_t key_token = json->tokens[token_index++];
        jsmntok_t value_token = json->tokens[token_index];
        if (key_token.start > object_token.end) {
            break;
        }
        if (key_token.start <= prev_element_end) {
            continue;
        }
        prev_element_end = value_token.end;
        element_count++;
    }

    return element_count;
}

int16_t object_get_nth_key(uint16_t object_token_index,
                           uint16_t object_element_index,
                           const parsed_json_t *parsed_transaction) {
    if (object_token_index < 0 || object_token_index > parsed_transaction->numberOfTokens) {
        return -1;
    }

    jsmntok_t object_token = parsed_transaction->tokens[object_token_index];
    uint16_t token_index = object_token_index;
    uint16_t element_count = 0;
    uint16_t prev_element_end = object_token.start;
    token_index++;
    while (true) {
        if (token_index >= parsed_transaction->numberOfTokens) {
            break;
        }
        jsmntok_t key_token = parsed_transaction->tokens[token_index++];
        jsmntok_t value_token = parsed_transaction->tokens[token_index];
        if (key_token.start > object_token.end) {
            break;
        }
        if (key_token.start <= prev_element_end) {
            continue;
        }
        prev_element_end = value_token.end;
        if (element_count == object_element_index) {
            return token_index - 1;
        }
        element_count++;
    }

    return -1;
}

int16_t object_get_nth_value(uint16_t object_token_index,
                             uint16_t object_element_index,
                             const parsed_json_t *parsed_transaction) {
    if (object_token_index < 0 || object_token_index > parsed_transaction->numberOfTokens) {
        return -1;
    }

    int key_index = object_get_nth_key(object_token_index, object_element_index, parsed_transaction);
    if (key_index != -1) {
        return key_index + 1;
    }
    return -1;
}

int16_t object_get_value(const parsed_json_t *parsed_transaction,
                         uint16_t object_token_index,
                         const char *key_name) {
    if (object_token_index < 0 || object_token_index > parsed_transaction->numberOfTokens) {
        return -1;
    }

    const jsmntok_t object_token = parsed_transaction->tokens[object_token_index];

    int token_index = object_token_index;
    int prev_element_end = object_token.start;
    token_index++;

    while (token_index < parsed_transaction->numberOfTokens) {
        const jsmntok_t key_token = parsed_transaction->tokens[token_index];
        token_index++;
        const jsmntok_t value_token = parsed_transaction->tokens[token_index];

        if (key_token.start > object_token.end) {
            break;
        }
        if (key_token.start <= prev_element_end) {
            continue;
        }
        prev_element_end = value_token.end;

        if (((uint16_t) strlen(key_name)) == (key_token.end - key_token.start)) {
            if (EQUALS(key_name,
                       parsed_transaction->buffer + key_token.start,
                       key_token.end - key_token.start)) {
                return token_index;
            }
        }
    }

    return -1;
}
