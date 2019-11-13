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
#pragma once

#include "parser_common.h"
#include "json/json_parser.h"
#include "parser_txdef.h"

#ifdef __cplusplus
extern "C" {
#endif

extern parser_tx_t parser_tx_obj;

parser_error_t parser_init(parser_context_t *ctx,
                           const uint8_t *buffer,
                           uint16_t bufferSize);

parser_error_t _readTx(parser_context_t *c, parser_tx_t *v);

#ifdef __cplusplus
}
#endif
