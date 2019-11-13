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

#pragma once

#include <jsmn.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "parser_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(LEDGER_SPECIFIC)
#include "bolos_target.h"
#endif

/// Max number of accepted tokens in the JSON input
#define MAX_NUMBER_OF_TOKENS   1536

// we must limit the number
#if defined(TARGET_NANOS)
#undef MAX_NUMBER_OF_TOKENS
#define MAX_NUMBER_OF_TOKENS    128
#endif

#define ROOT_TOKEN_INDEX 0

//---------------------------------------------

// Context that keeps all the parsed data together. That includes:
//  - parsed json tokens
//  - re-created SendMsg struct with indices pointing to tokens in parsed json
typedef struct {
    int8_t isValid;
    uint16_t numberOfTokens;
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
parser_error_t json_parse_s(parsed_json_t *parsed_json,
                            const char *transaction,
                            uint16_t transaction_length);

/// Parse json to create a token representation
/// \param parsed_json
/// \param transaction
/// \return Error message
parser_error_t json_parse(parsed_json_t *parsed_json,
                          const char *transaction);

/// Get the number of elements in the array
/// \param array_token_index
/// \param json
/// \return number of elements
uint16_t array_get_element_count(uint16_t array_token_index,
                                 const parsed_json_t *json);

/// Get the token index of the nth array's element
/// \param array_token_index
/// \param element_index
/// \param json
/// \return returns the token index or -1 if not found
int16_t array_get_nth_element(uint16_t array_token_index,
                              uint16_t element_index,
                              const parsed_json_t *json);

/// Get the number of dictionary elements (key/value pairs) under given object
/// \param object_token_index: token index of the parent object
/// \param parsed_transaction
/// \return number of elements
uint16_t object_get_element_count(uint16_t object_token_index,
                                  const parsed_json_t *parsed_transaction);

/// Get the token index for the nth dictionary key
/// \param object_token_index: token index of the parent object
/// \param object_element_index
/// \param parsed_transaction
/// \return returns token index or -1 if not found
int16_t object_get_nth_key(uint16_t object_token_index,
                           uint16_t object_element_index,
                           const parsed_json_t *parsed_transaction);

/// Get the token index for the nth dictionary value
/// \param object_token_index: token index of the parent object
/// \param object_element_index
/// \param parsed_transaction
/// \return returns token index or -1 if not found
int16_t object_get_nth_value(uint16_t object_token_index,
                             uint16_t object_element_index,
                             const parsed_json_t *parsed_transaction);

/// Get the token index of the value that matches the given key
/// \param object_token_index: token index of the parent object
/// \param key_name: key name of the wanted value
/// \param parsed_transaction
/// \param transaction
/// \return returns token index or -1 if not found
int16_t object_get_value(const parsed_json_t *parsed_transaction,
                         uint16_t object_token_index,
                         const char *key_name);

#ifdef __cplusplus
}
#endif
