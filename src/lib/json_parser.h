/*******************************************************************************
*   (c) 2018 ZondaX GmbH
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

#ifndef CI_TEST_JSONPARSER_H
#define CI_TEST_JSONPARSER_H

#include <jsmn.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char byte;

/// Max number of accepted tokens in the JSON input
#define MAX_NUMBER_OF_TOKENS    128

//---------------------------------------------

// Context that keeps all the parsed data together. That includes:
//  - parsed json tokens
//  - re-created SendMsg struct with indices pointing to tokens in parsed json
typedef struct {
    // Tokens
    bool CorrectFormat;
    byte NumberOfTokens;
    jsmntok_t Tokens[MAX_NUMBER_OF_TOKENS];
} parsed_json_t;

typedef struct {
    const parsed_json_t *parsed_transaction;
    unsigned short max_chars_per_key_line;
    unsigned short max_chars_per_value_line;
    const char *transaction;
} parsing_context_t;

//---------------------------------------------
// NEW JSON PARSER CODE

/// Parse json to create a token representation
/// \param parsed_json
/// \param transaction
void json_parse(parsed_json_t *parsed_json,
                const char *transaction);

/// Get number of elements in array
/// \param array_token_index
/// \param parsed_transaction
/// \return
int array_get_element_count(int array_token_index,
                            const parsed_json_t *parsed_transaction);

/// Get token index of the nth array's element
/// \param array_token_index
/// \param element_index
/// \param parsed_transaction
/// \return
int array_get_nth_element(int array_token_index,
                          int element_index,
                          const parsed_json_t *parsed_transaction);

/// Get number of elements (key/value pairs) in object
/// \param object_token_index
/// \param parsed_transaction
/// \return
int object_get_element_count(int object_token_index,
                             const parsed_json_t *parsed_transaction);

/// Get token index for the nth key
/// \param object_token_index
/// \param object_element_index
/// \param parsed_transaction
/// \return
int object_get_nth_key(int object_token_index,
                       int object_element_index,
                       const parsed_json_t *parsed_transaction);

/// Get token index for the nth value
/// \param object_token_index
/// \param object_element_index
/// \param parsed_transaction
/// \return
int object_get_nth_value(int object_token_index,
                         int object_element_index,
                         const parsed_json_t *parsed_transaction);

/// Get token index for the value that matched given key
/// \param object_token_index
/// \param key_name
/// \param parsed_transaction
/// \param transaction
/// \return
int object_get_value(int object_token_index,
                     const char *key_name,
                     const parsed_json_t *parsed_transaction,
                     const char *transaction);


#ifdef __cplusplus
}
#endif
#endif //CI_TEST_JSONPARSER_H
