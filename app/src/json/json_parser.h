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

#pragma once

#include <jsmn.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "common/parser_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(LEDGER_SPECIFIC)
#include "bolos_target.h"
#endif

/// Max number of accepted tokens in the JSON input
#define MAX_NUMBER_OF_TOKENS   768

// we must limit the number
#if defined(TARGET_NANOS)
#undef MAX_NUMBER_OF_TOKENS
#define MAX_NUMBER_OF_TOKENS    70
#endif

#if defined(TARGET_STAX)
#undef MAX_NUMBER_OF_TOKENS
#define MAX_NUMBER_OF_TOKENS    600
#endif

#define ROOT_TOKEN_INDEX 0

//---------------------------------------------

// Context that keeps all the parsed data together. That includes:
//  - parsed json tokens
//  - re-created SendMsg struct with indices pointing to tokens in parsed json
typedef struct {
    uint8_t isValid;
    uint32_t numberOfTokens;
    jsmntok_t tokens[MAX_NUMBER_OF_TOKENS];
    const char *buffer;
    uint16_t bufferLen;
} parsed_json_t;

//---------------------------------------------
// NEW JSON PARSER CODE

/// Parse json to create a token representation
/// \param parsed_json
/// \param transaction
/// \param transaction_length
/// \return Error message
parser_error_t json_parse(parsed_json_t *parsed_json,
                          const char *transaction,
                          uint16_t transaction_length);

/// Get the number of elements in the array
/// \param json
/// \param array_token_index
/// \param number of elements (out)
/// \return Error message
parser_error_t array_get_element_count(const parsed_json_t *json,
                                       uint16_t array_token_index,
                                       uint16_t *number_elements);

/// Get the token index of the nth array's element
/// \param json
/// \param array_token_index
/// \param element_index
/// \param token index
/// \return Error message
parser_error_t array_get_nth_element(const parsed_json_t *json,
                                     uint16_t array_token_index,
                                     uint16_t element_index,
                                     uint16_t *token_index);

/// Get the number of dictionary elements (key/value pairs) under given object
/// \param json
/// \param object_token_index: token index of the parent object
/// \param number of elements (out)
/// \return Error message
parser_error_t object_get_element_count(const parsed_json_t *json,
                                        uint16_t object_token_index,
                                        uint16_t *number_elements);

/// Get the token index for the nth dictionary key
/// \param json
/// \param object_token_index: token index of the parent object
/// \param object_element_index
/// \return token index (out)
/// \return Error message
parser_error_t object_get_nth_key(const parsed_json_t *json,
                                  uint16_t object_token_index,
                                  uint16_t object_element_index,
                                  uint16_t *token_index);

/// Get the token index for the nth dictionary value
/// \param json
/// \param object_token_index: token index of the parent object
/// \param object_element_index
/// \return token index (out))
/// \return Error message
parser_error_t object_get_nth_value(const parsed_json_t *json,
                                    uint16_t object_token_index,
                                    uint16_t object_element_index,
                                    uint16_t *token_index);

/// Get the token index of the value that matches the given key
/// \param json
/// \param object_token_index: token index of the parent object
/// \param key_name: key name of the wanted value
/// \return Error message
parser_error_t object_get_value(const parsed_json_t *json,
                                uint16_t object_token_index,
                                const char *key_name,
                                uint16_t *token_index);

#ifdef __cplusplus
}
#endif
