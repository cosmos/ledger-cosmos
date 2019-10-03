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

#include "json_parser.h"
#include <stdint.h>
#include <parser_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Validate json transaction
/// \param parsed_transacton
/// \param transaction
/// \return
parser_error_t tx_validate(parsed_json_t *json);

#ifdef __cplusplus
}
#endif
