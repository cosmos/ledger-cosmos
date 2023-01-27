/*******************************************************************************
*   (c) 2019 Zondax GmbH
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

#ifdef __cplusplus
extern "C" {
#endif

#include "parser_impl.h"
#include "coin.h"

#define OUTPUT_HANDLER_SIZE 600
#if defined(TARGET_NANOS)
#undef OUTPUT_HANDLER_SIZE
#define OUTPUT_HANDLER_SIZE 500
#endif

#define MAX_CONTENT_SIZE 600
#define MAX_TITLE_SIZE 40
#define PRINTABLE_TITLE_SIZE 17
#define PRINTABLE_PAGINATED_TITLE_SIZE 10
#define SCREEN_BREAK ":"
#define SCREEN_INDENT ">"
#define TITLE_TRUNCATE_REPLACE "---"
#define END_OF_STRING_SIZE

const char *parser_getErrorDescription(parser_error_t err);

//// parses a tx buffer
parser_error_t parser_parse(parser_context_t *ctx,
                            const uint8_t *data,
                            size_t dataLen,
                            parser_tx_t *tx_obj);

//// verifies tx fields
parser_error_t parser_validate(const parser_context_t *ctx);

//// returns the number of items in the current parsing context
parser_error_t parser_getNumItems(const parser_context_t *ctx, uint8_t *num_items);

// retrieves a readable output for each field / page
parser_error_t parser_getItem(const parser_context_t *ctx,
                              uint8_t displayIdx,
                              char *outKey, uint16_t outKeyLen,
                              char *outVal, uint16_t outValLen,
                              uint8_t pageIdx, uint8_t *pageCount);

#ifdef __cplusplus
}
#endif
