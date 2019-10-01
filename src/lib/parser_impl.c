/*******************************************************************************
*  (c) 2019 ZondaX GmbH
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

#include "parser_impl.h"
#include "json/tx_display.h"

parser_tx_t parser_tx_obj;
const char *lastErrorMessage = NULL;

parser_error_t parser_init_context(parser_context_t *ctx,
                                   const uint8_t *buffer,
                                   uint16_t bufferSize) {
    ctx->offset = 0;

    if (bufferSize == 0 || buffer == NULL) {
        // Not available, use defaults
        ctx->buffer = NULL;
        ctx->bufferSize = 0;
        return parser_no_data;
    }

    ctx->buffer = buffer;
    ctx->bufferSize = bufferSize;

    return parser_ok;
}

parser_error_t parser_init(parser_context_t *ctx, const uint8_t *buffer, uint16_t bufferSize) {
    parser_error_t err = parser_init_context(ctx, buffer, bufferSize);
    if (err != parser_ok)
        return err;
    return err;
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

parser_error_t _readTx(parser_context_t *c, parser_tx_t *v) {
    lastErrorMessage = json_parse_s(&parser_tx_obj.json, (const char *) c->buffer, c->bufferSize);
    if (lastErrorMessage != NULL) {
        return parser_extended_error;
    }

    parser_tx_obj.tx = (const char *) c->buffer;
    parser_tx_obj.cache_valid = 0;

    return parser_ok;
}
