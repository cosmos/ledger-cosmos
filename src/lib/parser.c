/*******************************************************************************
*   (c) 2019 ZondaX GmbH
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

#include <stdio.h>
#include <zxmacros.h>
#include "parser.h"
#include "cosmos.h"

#include "lib/json_parser.h"
#include "lib/tx_validate.h"
#include "lib/tx_parser.h"
#include "lib/tx_display.h"

#include "lib/parser_impl.h"
#include "view_internal.h"

parsed_json_t parsed_transaction;
const char *lastErrorMessage = NULL;

parser_error_t parser_parse(parser_context_t *ctx,
                            uint8_t *data,
                            uint16_t dataLen) {

    lastErrorMessage = json_parse_s(&parsed_transaction, (const char *) data, dataLen);
    if (lastErrorMessage != NULL) {
        return parser_extended_error;
    }
    lastErrorMessage = json_validate(&parsed_transaction, (const char *) data);
    if (lastErrorMessage != NULL) {
        return parser_extended_error;
    }

    parsing_context_t context;
    context.tx = (const char *) data;
    context.max_chars_per_key_line = MAX_CHARS_PER_KEY_LINE;
    context.max_chars_per_value_line = MAX_CHARS_PER_VALUE_LINE;
    context.parsed_tx = &parsed_transaction;

    set_parsing_context(context);
    tx_display_index_root();

    return parser_ok;
}

const char *parser_getErrorDescription(parser_error_t err) {
    switch (err) {
        case parser_ok:
            return "No error";
        case parser_no_data:
            return "No more data";
        case parser_extended_error:
            if (lastErrorMessage != NULL)
                return lastErrorMessage;
            return "Unknown message";
        case parser_unexpected_buffer_end:
            return "Unexpected buffer end";
        case parser_unexpected_wire_type:
            return "Unexpected wire type";
        case parser_unexpected_version:
            return "Unexpected version";
        case parser_unexpected_characters:
            return "Unexpected characters";
        case parser_unexpected_field:
            return "Unexpected field";
        case parser_duplicated_field:
            return "Unexpected duplicated field";
        case parser_unexpected_chain:
            return "Unexpected chain";
        default:
            return "Unrecognized error code";
    }
}

parser_error_t parser_validate() {
    return parser_ok;
}

uint8_t parser_getNumItems(parser_context_t *ctx) {
    return tx_display_num_pages();
}

parser_error_t parser_getItem(parser_context_t *ctx,
                              int8_t displayIdx,
                              char *outKey, uint16_t outKeyLen,
                              char *outValue, uint16_t outValueLen,
                              uint8_t pageIdx, uint8_t *pageCount) {

    snprintf(outKey, outKeyLen, "?");
    snprintf(outValue, outValueLen, "?");

    parser_error_t err = parser_ok;

    INIT_QUERY(outKey, outKeyLen, outValue, outValueLen, displayIdx)
    * pageCount = tx_display_get_item(pageIdx);
    tx_display_make_friendly();

    return err;
}
