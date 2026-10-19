/*******************************************************************************
 *   (c) 2026 Zondax AG
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

#include <gmock/gmock.h>

#include "common/parser.h"
#include "common/parser_common.h"
#include <string>

// Every refusal the parser can reach is reported to the host through
// parser_getErrorDescription. A code with no case of its own falls through to
// "Unrecognized error code", which tells whoever is holding the device nothing
// about why their transaction was refused -- the item-count refusal reached the
// host that way. The default belongs to values outside the enum, not to members
// of it.
namespace {

const char *kUnrecognized = "Unrecognized error code";

}  // namespace

TEST(ErrorDescriptions, EveryErrorCodeIsNamed) {
  for (int code = parser_ok; code <= parser_swap_multiple_coins; code++) {
    const char *desc = parser_getErrorDescription((parser_error_t)code);
    ASSERT_NE(desc, nullptr) << "code " << code;
    EXPECT_STRNE(desc, kUnrecognized)
        << "parser_error_t value " << code << " has no description";
    EXPECT_FALSE(std::string(desc).empty()) << "code " << code;
  }
}

TEST(ErrorDescriptions, OutOfRangeCodeStillFallsThrough) {
  EXPECT_STREQ(parser_getErrorDescription((parser_error_t)(
                   parser_swap_multiple_coins + 1)),
               kUnrecognized);
}

TEST(ErrorDescriptions, ItemCountRefusalIsNamed) {
  EXPECT_STREQ(parser_getErrorDescription(parser_unexpected_number_items),
               "Unexpected number of items");
}
