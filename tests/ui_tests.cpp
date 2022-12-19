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

#include <gmock/gmock.h>
#include "testcases.h"

#include <iostream>
#include <memory>
#include "common/parser.h"
#include "common.h"
#include "app_mode.h"
#include "coin.h"
#include <hexutils.h>


using ::testing::TestWithParam;
using ::testing::Values;

void validate_testcase(const testcase_t &tc) {
    parser_context_t ctx;
    parser_error_t err;

    const auto *buffer = (const uint8_t *) tc.tx.c_str();
    size_t bufferLen = tc.tx.size();

    parser_tx_t tx_obj;
    memset(&tx_obj, 0, sizeof(tx_obj));
    tx_obj.tx_type = tx_json;
    err = parser_parse(&ctx, buffer, bufferLen, &tx_obj);
    ASSERT_EQ(parser_getErrorDescription(err), tc.parsingErr) << "Parsing error mismatch";

    if (err != parser_ok)
        return;

    err = parser_validate(&ctx);
    EXPECT_EQ( parser_getErrorDescription(err), tc.validationErr) << "Validation error mismatch";
}

void check_testcase(const testcase_t &tc) {
    parser_context_t ctx;
    parser_error_t err;

    app_mode_set_expert(tc.expert);

    const auto *buffer = (const uint8_t *) tc.tx.c_str();
    size_t bufferLen = tc.tx.size();

    parser_tx_t tx_obj;
    memset(&tx_obj, 0, sizeof(tx_obj));
    tx_obj.tx_type = tx_json;
    err = parser_parse(&ctx, buffer, bufferLen, &tx_obj);
    ASSERT_EQ(parser_getErrorDescription(err), tc.parsingErr)  << "Parsing error mismatch";

    if (err != parser_ok)
        return;

    auto output = dumpUI(&ctx, 40, 40);

    for (const auto &i : output) {
        std::cout << i << std::endl;
    }
    std::cout << std::endl << std::endl;

    EXPECT_EQ(output.size(), tc.expected.size());
    for (size_t i = 0; i < tc.expected.size(); i++) {
        if (i < output.size()) {
            EXPECT_THAT(output[i], testing::Eq(tc.expected[i]));
        }
    }
}

void check_Textualtestcase(const testcase_t &tc, bool expert_mode) {
    parser_context_t ctx;
    parser_error_t err;

    app_mode_set_expert(expert_mode);

    uint8_t buffer[10000];
    uint16_t bufferLen = parseHexString(buffer, sizeof(buffer), tc.tx.c_str());

    parser_tx_t tx_obj;
    memset(&tx_obj, 0, sizeof(tx_obj));
    tx_obj.tx_type = tx_textual;
    err = parser_parse(&ctx, buffer, bufferLen, &tx_obj);
    ASSERT_EQ(err, parser_ok) << parser_getErrorDescription(err);

    if (err != parser_ok)
        return;

    auto output = dumpUI(&ctx, 40, 40);

    for (const auto &i : output) {
        std::cout << i << std::endl;
    }
    std::cout << std::endl << std::endl;

    std::vector<std::string> expected = tc.expected;
    
    expected = app_mode_expert() ? tc.expected_expert : tc.expected;
    EXPECT_EQ(output.size(), expected.size());
    for (size_t i = 0; i < expected.size(); i++) {
        if (i < output.size()) {
            EXPECT_THAT(output[i], testing::Eq(expected[i]));
        }
    }
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

class JsonTests : public ::testing::TestWithParam<testcase_t> {
public:
    struct PrintToStringParamName {
        template<class ParamType>
        std::string operator()(const testing::TestParamInfo<ParamType> &info) const {
            auto p = static_cast<testcase_t>(info.param);
            std::stringstream ss;
            ss << p.name;
            return ss.str();
        }
    };
};

class JsonTests_Secp256 : public JsonTests {};
class JsonTests_Textual : public JsonTests {};

INSTANTIATE_TEST_SUITE_P ( // NOLINT(cert-err58-cpp)
    JsonTestCases,
    JsonTests_Secp256,
    ::testing::ValuesIn(GetJsonTestCases("testcases/manual.json")),
    JsonTests_Secp256::PrintToStringParamName()
);

INSTANTIATE_TEST_SUITE_P ( // NOLINT(cert-err58-cpp)
    JsonTestTextualCases,
    JsonTests_Textual,
    ::testing::ValuesIn(GetJsonTextualTestCases("testcases/textual.json")),
    JsonTests_Textual::PrintToStringParamName()
);

TEST_P(JsonTests_Secp256, ValidateTestcase) { validate_testcase(GetParam()); }
TEST_P(JsonTests_Secp256, CheckUIOutput) { check_testcase(GetParam()); }

TEST_P(JsonTests_Textual, Normal) { check_Textualtestcase(GetParam(), false); }
TEST_P(JsonTests_Textual, Expert) { check_Textualtestcase(GetParam(), true); }
