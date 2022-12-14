/*******************************************************************************
*   (c) 2018 Zondax GmbH
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

#include <json/json_parser.h>
#include <parser_impl.h>
#include <coin.h>
#include <vector>

#define EXPECT_EQ_STR(_STR1, _STR2, _ERROR_MESSAGE) { if ((_STR1) != nullptr & (_STR2) != nullptr) \
EXPECT_TRUE(!strcmp(_STR1, _STR2)) << (_ERROR_MESSAGE) << ", expected: " << (_STR2) << ", received: " << (_STR1); \
else FAIL() << "One of the strings is null"; }

parser_error_t parse_tx(parsed_json_t *parsed_json, const char *tx);

std::vector<std::string> dumpUI(parser_context_t *ctx, uint16_t maxKeyLen, uint16_t maxValueLen);

#define JSON_PARSE(parsed_json, buffer) json_parse(parsed_json, buffer, strlen(buffer))
