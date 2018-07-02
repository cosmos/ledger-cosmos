/*******************************************************************************
*   (c) 2018 ZondaX GmbH
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

#include "gtest/gtest.h"
#include "lib/json_parser.h"
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <jsmn.h>

namespace {
std::string exec(const char *cmd) {
    std::array<char, 128> buffer{};
    std::string result;
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe)
        throw std::runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
            result += buffer.data();
    }
    return result;
}

TEST(JsonParserTest, Empty) {
    parsed_json_t parserData = {false};
    json_parse(&parserData, "");

    EXPECT_FALSE(parserData.CorrectFormat);
    EXPECT_EQ(0, parserData.NumberOfTokens);
}

TEST(JsonParserTest, SinglePrimitive) {
    parsed_json_t parserData = {false};
    json_parse(&parserData, "EMPTY");

    EXPECT_TRUE(parserData.CorrectFormat);
    EXPECT_EQ(1, parserData.NumberOfTokens);
    EXPECT_TRUE(parserData.Tokens[0].type == jsmntype_t::JSMN_PRIMITIVE);
}

TEST(JsonParserTest, KeyValuePrimitives) {
    parsed_json_t parserData = {false};
    json_parse(&parserData, "KEY : VALUE");

    EXPECT_TRUE(parserData.CorrectFormat);
    EXPECT_EQ(2, parserData.NumberOfTokens);
    EXPECT_TRUE(parserData.Tokens[0].type == jsmntype_t::JSMN_PRIMITIVE);
    EXPECT_TRUE(parserData.Tokens[1].type == jsmntype_t::JSMN_PRIMITIVE);
}

TEST(JsonParserTest, SingleString) {
    parsed_json_t parserData = {false};
    json_parse(&parserData, "\"EMPTY\"");

    EXPECT_TRUE(parserData.CorrectFormat);
    EXPECT_EQ(1, parserData.NumberOfTokens);
    EXPECT_TRUE(parserData.Tokens[0].type == jsmntype_t::JSMN_STRING);
}

TEST(JsonParserTest, KeyValueStrings) {
    parsed_json_t parserData = {false};
    json_parse(&parserData, R"("KEY" : "VALUE")");

    EXPECT_TRUE(parserData.CorrectFormat);
    EXPECT_EQ(2, parserData.NumberOfTokens);
    EXPECT_TRUE(parserData.Tokens[0].type == jsmntype_t::JSMN_STRING);
    EXPECT_TRUE(parserData.Tokens[1].type == jsmntype_t::JSMN_STRING);
}

TEST(JsonParserTest, SimpleArray) {
    parsed_json_t parserData = {false};
    json_parse(&parserData, "LIST : [1, 2, 3, 4]");

    EXPECT_TRUE(parserData.CorrectFormat);
    EXPECT_EQ(6, parserData.NumberOfTokens);
    EXPECT_TRUE(parserData.Tokens[0].type == jsmntype_t::JSMN_PRIMITIVE);
    EXPECT_TRUE(parserData.Tokens[1].type == jsmntype_t::JSMN_ARRAY);
    EXPECT_TRUE(parserData.Tokens[2].type == jsmntype_t::JSMN_PRIMITIVE);
    EXPECT_TRUE(parserData.Tokens[3].type == jsmntype_t::JSMN_PRIMITIVE);
    EXPECT_TRUE(parserData.Tokens[4].type == jsmntype_t::JSMN_PRIMITIVE);
    EXPECT_TRUE(parserData.Tokens[5].type == jsmntype_t::JSMN_PRIMITIVE);
}

TEST(JsonParserTest, MixedArray) {
    parsed_json_t parserData = {false};
    json_parse(&parserData, R"(LIST : [1, "Text", 3, "Another text"])");

    EXPECT_TRUE(parserData.CorrectFormat);
    EXPECT_EQ(6, parserData.NumberOfTokens);
    EXPECT_TRUE(parserData.Tokens[0].type == jsmntype_t::JSMN_PRIMITIVE);
    EXPECT_TRUE(parserData.Tokens[1].type == jsmntype_t::JSMN_ARRAY);
    EXPECT_TRUE(parserData.Tokens[2].type == jsmntype_t::JSMN_PRIMITIVE);
    EXPECT_TRUE(parserData.Tokens[3].type == jsmntype_t::JSMN_STRING);
    EXPECT_TRUE(parserData.Tokens[4].type == jsmntype_t::JSMN_PRIMITIVE);
    EXPECT_TRUE(parserData.Tokens[5].type == jsmntype_t::JSMN_STRING);
}

TEST(JsonParserTest, SimpleObject) {
    parsed_json_t parserData = {false};
    json_parse(&parserData, "vote : "
                            "{ "
                            "\"key\" : \"value\", "
                            "\"another key\" : { "
                            "\"inner key\" : \"inner value\", "
                            "\"total\":123 }"
                            "}");

    EXPECT_TRUE(parserData.CorrectFormat);
    EXPECT_EQ(10, parserData.NumberOfTokens);
    EXPECT_TRUE(parserData.Tokens[0].type == jsmntype_t::JSMN_PRIMITIVE);
    EXPECT_TRUE(parserData.Tokens[1].type == jsmntype_t::JSMN_OBJECT);
    EXPECT_TRUE(parserData.Tokens[2].type == jsmntype_t::JSMN_STRING);
    EXPECT_TRUE(parserData.Tokens[3].type == jsmntype_t::JSMN_STRING);
    EXPECT_TRUE(parserData.Tokens[4].type == jsmntype_t::JSMN_STRING);
    EXPECT_TRUE(parserData.Tokens[5].type == jsmntype_t::JSMN_OBJECT);
    EXPECT_TRUE(parserData.Tokens[6].type == jsmntype_t::JSMN_STRING);
    EXPECT_TRUE(parserData.Tokens[7].type == jsmntype_t::JSMN_STRING);
    EXPECT_TRUE(parserData.Tokens[8].type == jsmntype_t::JSMN_STRING);
    EXPECT_TRUE(parserData.Tokens[9].type == jsmntype_t::JSMN_PRIMITIVE);
}
}