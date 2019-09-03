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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

typedef enum {
    parser_ok = 0,
    parser_no_data = 1,
    parser_extended_error = 2,
    parser_unexpected_buffer_end = 3,
    parser_unexpected_wire_type = 4,
    parser_unexpected_version = 5,
    parser_unexpected_characters = 6,
    parser_unexpected_field = 7,
    parser_duplicated_field = 8,
    parser_value_out_of_range = 9,
    parser_unexpected_chain = 10,
} parser_error_t;

typedef struct {
    const char *data;
    uint16_t dataLen;
    struct {
        unsigned make_friendly :1;
    } flags;
} parser_context_t;

#ifdef __cplusplus
}
#endif
