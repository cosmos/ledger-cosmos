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

#ifndef CI_TEST_TRANSACTIONPARSER_H
#define CI_TEST_TRANSACTIONPARSER_H

#include "json_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Update value characters from json transaction read from the token_index element.
/// Value is only updated if current_item_index (which is incremented internally) matches item_index_to_display
/// If value is updated, we also update view_scrolling_total_size to value string length.
// \param value
/// \param value_length
/// \param token_index
/// \param current_item_index
/// \param item_index_to_display
/// \param chunk_index
/// \return
int display_value(char *value,
                  int value_length,
                  int token_index,
                  int *current_item_index,
                  int item_index_to_display,
                  int *chunk_index);

/// Update key characters from json transaction read from the token_index element.
/// \param key
/// \param key_length
/// \param token_index
void display_key(char *key,
                 int key_length,
                 int token_index);

/// Generic function to display arbitrary json based on the specification
/// \param item_index_to_display
/// \param key
/// \param key_length
/// \param value
/// \param value_length
/// \param token_index
/// \param chunk_index
/// \return
int display_arbitrary_item(int item_index_to_display,
                           char *key,
                           int key_length,
                           char *value,
                           int value_length,
                           int token_index,
                           int *chunk_index);

/// display_get_arbitrary_items_count
/// \param token_index
/// \return
int display_get_arbitrary_items_count(int token_index);

/// transaction_get_display_key_value
/// \param key
/// \param key_length
/// \param value
/// \param value_length
/// \param page_index
/// \param chunk_index
/// \return
int transaction_get_display_key_value(char *key,
                                      int key_length,
                                      char *value,
                                      int value_length,
                                      int page_index,
                                      int *chunk_index);

/// transaction_get_display_pages
/// \return
int transaction_get_display_pages();

/// Validate json transaction
/// \param parsed_transacton
/// \param transaction
/// \return
const char* json_validate(
        parsed_json_t* parsed_transaction,
        const char *transaction);

//---------------------------------------------
// Delegates

typedef void(*copy_delegate)(void *dst, const void *source, size_t size);
void set_copy_delegate(copy_delegate delegate);
void set_parsing_context(parsing_context_t context);

//---------------------------------------------

#ifdef __cplusplus
}
#endif
#endif //CI_TEST_TRANSACTIONPARSER_H
