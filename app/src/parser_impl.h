/*******************************************************************************
*  (c) 2019 Zondax GmbH
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

#include "parser_common.h"
#include <zxmacros.h>
#include "zxtypes.h"
#include "json/json_parser.h"
#include "parser_txdef.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const uint8_t *buffer;
    uint16_t bufferLen;
    uint16_t offset;
    parser_tx_t *tx_obj;
} parser_context_t;

typedef struct {
    char str1[50];
    char str2[50];
} key_subst_t;

typedef struct {
    char ascii_code;
    char str;
} ascii_subst_t;

extern parser_tx_t parser_tx_obj;

parser_error_t _read_json_tx(parser_context_t *c, parser_tx_t *v);
parser_error_t _read_text_tx(parser_context_t *c, parser_tx_t *v);

#ifdef __cplusplus
}
#endif
