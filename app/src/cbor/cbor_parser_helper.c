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

#include "cbor.h"
#include "cbor_parser_helper.h"
#include "parser_txdef.h"
#include <zxmacros.h>
#include <common/parser_common.h>
#include "parser.h"

parser_error_t parser_mapCborError(CborError err) {
    switch (err) {
        case CborErrorUnexpectedEOF:
            return parser_cbor_unexpected_EOF;
        case CborErrorMapNotSorted:
            return parser_cbor_not_canonical;
        case CborNoError:
            return parser_ok;
        default:
            return parser_cbor_unexpected;
    }
}

static parser_error_t cbor_check_optFields(CborValue *data, Cbor_container *container) {
    int key;
    for (size_t i = 0; i < container->n_field; i++) {

        PARSER_ASSERT_OR_ERROR(cbor_value_is_integer(data), parser_unexpected_type)
        CHECK_CBOR_MAP_ERR(cbor_value_get_int(data, &key))
        CHECK_CBOR_MAP_ERR(cbor_value_advance(data))

        switch(key) {
            case INDENT_KEY_ID: {
                int tmpVal = 0;
                PARSER_ASSERT_OR_ERROR(cbor_value_is_integer(data), parser_unexpected_type)
                CHECK_CBOR_MAP_ERR(cbor_value_get_int(data, &tmpVal))
                PARSER_ASSERT_OR_ERROR((tmpVal >= 0 && tmpVal <= UINT8_MAX), parser_unexpected_value)
                container->screen.indent = (uint8_t) tmpVal;
                break;
            }

            case EXPERT_KEY_ID:
                PARSER_ASSERT_OR_ERROR(cbor_value_is_boolean(data), parser_unexpected_type)
                CHECK_CBOR_MAP_ERR(cbor_value_get_boolean(data, &container->screen.expert))
            break;

            default:
                container->screen.indent = 0;
                container->screen.expert = false;
        }
        CHECK_CBOR_MAP_ERR(cbor_value_advance(data))
    }
    return parser_ok;
}

static parser_error_t cbor_check_screen(CborValue *data, Cbor_container *container) {
    int screen_key;
    //check title Key
    PARSER_ASSERT_OR_ERROR(cbor_value_is_integer(data), parser_unexpected_type)
    CHECK_CBOR_MAP_ERR(cbor_value_get_int(data, &screen_key))
    if (screen_key != TITLE_KEY_ID) {
        PARSER_ASSERT_OR_ERROR(screen_key==CONTENT_KEY_ID, parser_unexpected_type)
        
        // No title
        container->screen.titlePtr = NULL;
        container->screen.titleLen = 0;
        CHECK_CBOR_MAP_ERR(cbor_value_advance(data))

        //get content ptr
        READ_STRING_PTR_SIZE(data, container->screen.contentPtr, container->screen.contentLen)
        PARSER_ASSERT_OR_ERROR(container->screen.contentLen <= MAX_CONTENT_SIZE, parser_unexpected_value)
        return parser_ok;
    }

    CHECK_CBOR_MAP_ERR(cbor_value_advance(data))

    //get title ptr
    READ_STRING_PTR_SIZE(data, container->screen.titlePtr, container->screen.titleLen)
    PARSER_ASSERT_OR_ERROR(container->screen.titleLen <= MAX_CONTENT_SIZE, parser_unexpected_value)

    CHECK_CBOR_MAP_ERR(cbor_value_advance(data));

    //check content Key
    PARSER_ASSERT_OR_ERROR(cbor_value_is_integer(data), parser_unexpected_type)
    CHECK_CBOR_MAP_ERR(cbor_value_get_int(data, &screen_key))
    PARSER_ASSERT_OR_ERROR(screen_key==CONTENT_KEY_ID, parser_unexpected_type)

    CHECK_CBOR_MAP_ERR(cbor_value_advance(data))

    //get content ptr
    READ_STRING_PTR_SIZE(data, container->screen.contentPtr, container->screen.contentLen)
    PARSER_ASSERT_OR_ERROR(container->screen.contentLen <= MAX_CONTENT_SIZE, parser_unexpected_value)

    return parser_ok;
}

parser_error_t cbor_get_containerInfo(CborValue *data, Cbor_container *container) {
    if (data == NULL || container == NULL) {
        return parser_unexpected_value;
    }

    PARSER_ASSERT_OR_ERROR(!cbor_value_at_end(data), parser_unexpected_buffer_end)
    CHECK_PARSER_ERR(cbor_check_screen(data, container))
    CHECK_CBOR_MAP_ERR(cbor_value_advance(data))

    if (container->n_field > 2) {
        container->n_field -= 2;
        CHECK_PARSER_ERR(cbor_check_optFields(data, container))
    } else {
        container->screen.indent = 0;
        container->screen.expert = false;
    }

    return parser_ok;
}

parser_error_t cbor_check_expert(CborValue *data, Cbor_container *container) {
    if (data == NULL || container == NULL) {
        return parser_unexpected_value;
    }

    if (container->n_field > 1) {
        CHECK_PARSER_ERR(cbor_check_optFields(data, container))
    } else {
        container->screen.indent = 0;
        container->screen.expert = false;
    }

    return parser_ok;
}
