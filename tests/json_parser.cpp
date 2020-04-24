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

#include "gtest/gtest.h"
#include "util/common.h"
#include <jsmn.h>
#include <json/json_parser.h>

namespace {
    TEST(JsonParserTest, Empty) {
        parsed_json_t parserData = {false};
        JSON_PARSE(&parserData, "");

        EXPECT_FALSE(parserData.isValid);
        EXPECT_EQ(0, parserData.numberOfTokens);
    }

    TEST(JsonParserTest, SinglePrimitive) {
        parsed_json_t parserData = {false};
        JSON_PARSE(&parserData, "EMPTY");

        EXPECT_TRUE(parserData.isValid);
        EXPECT_EQ(1, parserData.numberOfTokens);
        EXPECT_TRUE(parserData.tokens[0].type == jsmntype_t::JSMN_PRIMITIVE);
    }

    TEST(JsonParserTest, KeyValuePrimitives) {
        parsed_json_t parserData = {false};
        JSON_PARSE(&parserData, "KEY : VALUE");

        EXPECT_TRUE(parserData.isValid);
        EXPECT_EQ(2, parserData.numberOfTokens);
        EXPECT_TRUE(parserData.tokens[0].type == jsmntype_t::JSMN_PRIMITIVE);
        EXPECT_TRUE(parserData.tokens[1].type == jsmntype_t::JSMN_PRIMITIVE);
    }

    TEST(JsonParserTest, SingleString) {
        parsed_json_t parserData = {false};
        JSON_PARSE(&parserData, "\"EMPTY\"");

        EXPECT_TRUE(parserData.isValid);
        EXPECT_EQ(1, parserData.numberOfTokens);
        EXPECT_TRUE(parserData.tokens[0].type == jsmntype_t::JSMN_STRING);
    }

    TEST(JsonParserTest, KeyValueStrings) {
        parsed_json_t parserData = {false};
        JSON_PARSE(&parserData, R"("KEY" : "VALUE")");

        EXPECT_TRUE(parserData.isValid);
        EXPECT_EQ(2, parserData.numberOfTokens);
        EXPECT_TRUE(parserData.tokens[0].type == jsmntype_t::JSMN_STRING);
        EXPECT_TRUE(parserData.tokens[1].type == jsmntype_t::JSMN_STRING);
    }

    TEST(JsonParserTest, SimpleArray) {
        parsed_json_t parserData = {false};
        JSON_PARSE(&parserData, "LIST : [1, 2, 3, 4]");

        EXPECT_TRUE(parserData.isValid);
        EXPECT_EQ(6, parserData.numberOfTokens);
        EXPECT_TRUE(parserData.tokens[0].type == jsmntype_t::JSMN_PRIMITIVE);
        EXPECT_TRUE(parserData.tokens[1].type == jsmntype_t::JSMN_ARRAY);
        EXPECT_TRUE(parserData.tokens[2].type == jsmntype_t::JSMN_PRIMITIVE);
        EXPECT_TRUE(parserData.tokens[3].type == jsmntype_t::JSMN_PRIMITIVE);
        EXPECT_TRUE(parserData.tokens[4].type == jsmntype_t::JSMN_PRIMITIVE);
        EXPECT_TRUE(parserData.tokens[5].type == jsmntype_t::JSMN_PRIMITIVE);
    }

    TEST(JsonParserTest, MixedArray) {
        parsed_json_t parserData = {false};
        JSON_PARSE(&parserData, R"(LIST : [1, "Text", 3, "Another text"])");

        EXPECT_TRUE(parserData.isValid);
        EXPECT_EQ(6, parserData.numberOfTokens);
        EXPECT_TRUE(parserData.tokens[0].type == jsmntype_t::JSMN_PRIMITIVE);
        EXPECT_TRUE(parserData.tokens[1].type == jsmntype_t::JSMN_ARRAY);
        EXPECT_TRUE(parserData.tokens[2].type == jsmntype_t::JSMN_PRIMITIVE);
        EXPECT_TRUE(parserData.tokens[3].type == jsmntype_t::JSMN_STRING);
        EXPECT_TRUE(parserData.tokens[4].type == jsmntype_t::JSMN_PRIMITIVE);
        EXPECT_TRUE(parserData.tokens[5].type == jsmntype_t::JSMN_STRING);
    }

    TEST(JsonParserTest, SimpleObject) {
        parsed_json_t parserData = {false};
        JSON_PARSE(&parserData, "vote : "
                                "{ "
                                "\"key\" : \"value\", "
                                "\"another key\" : { "
                                "\"inner key\" : \"inner value\", "
                                "\"total\":123 }"
                                "}");

        EXPECT_TRUE(parserData.isValid);
        EXPECT_EQ(10, parserData.numberOfTokens);
        EXPECT_TRUE(parserData.tokens[0].type == jsmntype_t::JSMN_PRIMITIVE);
        EXPECT_TRUE(parserData.tokens[1].type == jsmntype_t::JSMN_OBJECT);
        EXPECT_TRUE(parserData.tokens[2].type == jsmntype_t::JSMN_STRING);
        EXPECT_TRUE(parserData.tokens[3].type == jsmntype_t::JSMN_STRING);
        EXPECT_TRUE(parserData.tokens[4].type == jsmntype_t::JSMN_STRING);
        EXPECT_TRUE(parserData.tokens[5].type == jsmntype_t::JSMN_OBJECT);
        EXPECT_TRUE(parserData.tokens[6].type == jsmntype_t::JSMN_STRING);
        EXPECT_TRUE(parserData.tokens[7].type == jsmntype_t::JSMN_STRING);
        EXPECT_TRUE(parserData.tokens[8].type == jsmntype_t::JSMN_STRING);
        EXPECT_TRUE(parserData.tokens[9].type == jsmntype_t::JSMN_PRIMITIVE);
    }

    TEST(JsonParserTest, ArrayElementCount_objects) {
        auto transaction =
                R"({"array":[{"amount":5,"denom":"photon"}, {"amount":5,"denom":"photon"}, {"amount":5,"denom":"photon"}]})";

        parsed_json_t parsed_json;
        JSON_PARSE(&parsed_json, transaction);

        uint16_t token;
        EXPECT_EQ(array_get_element_count(&parsed_json, 2, &token), parser_ok);
        EXPECT_EQ(token, 3) << "Wrong number of array elements";
    }

    TEST(JsonParserTest, ArrayElementCount_primitives) {
        auto transaction = R"({"array":[1, 2, 3, 4, 5, 6, 7]})";

        parsed_json_t parsed_json;
        JSON_PARSE(&parsed_json, transaction);

        uint16_t token;
        EXPECT_EQ(array_get_element_count(&parsed_json, 2, &token), parser_ok);
        EXPECT_EQ(token, 7) << "Wrong number of array elements";
    }

    TEST(JsonParserTest, ArrayElementCount_strings) {
        auto transaction = R"({"array":["hello", "there"]})";

        parsed_json_t parsed_json;
        JSON_PARSE(&parsed_json, transaction);

        uint16_t token;
        EXPECT_EQ(array_get_element_count(&parsed_json, 2, &token), parser_ok);
        EXPECT_EQ(token, 2) << "Wrong number of array elements";
    }

    TEST(JsonParserTest, ArrayElementCount_empty) {
        auto transaction = R"({"array":[])";

        parsed_json_t parsed_json;
        JSON_PARSE(&parsed_json, transaction);

        uint16_t token;
        EXPECT_EQ(array_get_element_count(&parsed_json, 2, &token), parser_no_data);
    }

    TEST(JsonParserTest, ArrayElementGet_objects) {
        auto transaction =
                R"({"array":[{"amount":5,"denom":"photon"}, {"amount":5,"denom":"photon"}, {"amount":5,"denom":"photon"}]})";

        parsed_json_t parsed_json;
        JSON_PARSE(&parsed_json, transaction);

        uint16_t token_index;
        EXPECT_EQ(array_get_nth_element(&parsed_json, 2, 1, &token_index), parser_ok);
        EXPECT_EQ(token_index, 8) << "Wrong token index returned";
        EXPECT_EQ(parsed_json.tokens[token_index].type, JSMN_OBJECT) << "Wrong token type returned";
    }

    TEST(JsonParserTest, ArrayElementGet_primitives) {
        auto transaction = R"({"array":[1, 2, 3, 4, 5, 6, 7]})";

        parsed_json_t parsed_json;
        JSON_PARSE(&parsed_json, transaction);

        uint16_t token_index;
        EXPECT_EQ(array_get_nth_element(&parsed_json, 2, 5, &token_index), parser_ok);
        EXPECT_EQ(token_index, 8) << "Wrong token index returned";
        EXPECT_EQ(parsed_json.tokens[token_index].type, JSMN_PRIMITIVE) << "Wrong token type returned";
    }

    TEST(TxValidationTest, ArrayElementGet_strings) {
        auto transaction = R"({"array":["hello", "there"]})";

        parsed_json_t parsed_json;
        JSON_PARSE(&parsed_json, transaction);

        uint16_t token_index;
        EXPECT_EQ(array_get_nth_element(&parsed_json, 2, 0, &token_index), parser_ok);
        EXPECT_EQ(token_index, 3) << "Wrong token index returned";
        EXPECT_EQ(parsed_json.tokens[token_index].type, JSMN_STRING) << "Wrong token type returned";
    }

    TEST(TxValidationTest, ArrayElementGet_empty) {
        auto transaction = R"({"array":[])";

        parsed_json_t parsed_json;
        JSON_PARSE(&parsed_json, transaction);

        uint16_t token_index;
        EXPECT_EQ(array_get_nth_element(&parsed_json, 2, 0, &token_index), parser_no_data)
                            << "Token index should be invalid (not found).";
    }

    TEST(TxValidationTest, ArrayElementGet_out_of_bounds_negative) {
        auto transaction = R"({"array":["hello", "there"])";

        parsed_json_t parsed_json;
        JSON_PARSE(&parsed_json, transaction);

        uint16_t token_index;
        EXPECT_EQ(array_get_nth_element(&parsed_json, 2, -1, &token_index), parser_no_data)
                            << "Token index should be invalid (not found).";
    }

    TEST(TxValidationTest, ArrayElementGet_out_of_bounds) {
        auto transaction = R"({"array":["hello", "there"])";

        parsed_json_t parsed_json;
        JSON_PARSE(&parsed_json, transaction);

        uint16_t token_index;
        EXPECT_EQ(array_get_nth_element(&parsed_json, 2, 3, &token_index), parser_no_data)
                            << "Token index should be invalid (not found).";
    }

    TEST(TxValidationTest, ObjectElementCount_primitives) {
        auto transaction = R"({"age":36, "height":185, "year":1981})";

        parsed_json_t parsed_json;
        JSON_PARSE(&parsed_json, transaction);

        uint16_t count;
        EXPECT_EQ(object_get_element_count(&parsed_json, 0, &count), parser_ok);
        EXPECT_EQ(count, 3) << "Wrong number of object elements";
    }

    TEST(TxValidationTest, ObjectElementCount_string) {
        auto transaction = R"({"age":"36", "height":"185", "year":"1981", "month":"july"})";

        parsed_json_t parsed_json;
        JSON_PARSE(&parsed_json, transaction);

        uint16_t count;
        EXPECT_EQ(object_get_element_count(&parsed_json, 0, &count), parser_ok);
        EXPECT_EQ(count, 4) << "Wrong number of object elements";
    }

    TEST(TxValidationTest, ObjectElementCount_array) {
        auto transaction = R"({ "ages":[36, 31, 10, 2],
                            "heights":[185, 164, 154, 132],
                            "years":[1981, 1985, 2008, 2016],
                            "months":["july", "august", "february", "july"]})";

        parsed_json_t parsed_json;
        JSON_PARSE(&parsed_json, transaction);

        uint16_t count;
        EXPECT_EQ(object_get_element_count(&parsed_json, 0, &count), parser_ok);
        EXPECT_EQ(count, 4) << "Wrong number of object elements";
    }

    TEST(TxValidationTest, ObjectElementCount_object) {
        auto transaction = R"({"person1":{"age":36, "height":185, "year":1981},
                           "person2":{"age":36, "height":185, "year":1981},
                           "person3":{"age":36, "height":185, "year":1981}})";

        parsed_json_t parsed_json;
        JSON_PARSE(&parsed_json, transaction);

        uint16_t count;
        EXPECT_EQ(object_get_element_count(&parsed_json, 0, &count), parser_ok);
        EXPECT_EQ(count, 3) << "Wrong number of object elements";
    }

    TEST(TxValidationTest, ObjectElementCount_deep) {
        auto transaction = R"({"person1":{"age":{"age":36, "height":185, "year":1981}, "height":{"age":36, "height":185, "year":1981}, "year":1981},
                           "person2":{"age":{"age":36, "height":185, "year":1981}, "height":{"age":36, "height":185, "year":1981}, "year":1981},
                           "person3":{"age":{"age":36, "height":185, "year":1981}, "height":{"age":36, "height":185, "year":1981}, "year":1981}})";

        parsed_json_t parsed_json;
        JSON_PARSE(&parsed_json, transaction);

        uint16_t count;
        EXPECT_EQ(object_get_element_count(&parsed_json, 0, &count), parser_ok);
        EXPECT_EQ(count, 3) << "Wrong number of object elements";
    }

    TEST(TxValidationTest, ObjectElementGet_primitives) {
        auto transaction = R"({"age":36, "height":185, "year":1981})";

        parsed_json_t parsed_json;
        JSON_PARSE(&parsed_json, transaction);

        uint16_t token_index;
        EXPECT_EQ(object_get_nth_key(&parsed_json, 0, 0, &token_index), parser_ok);
        EXPECT_EQ(token_index, 1) << "Wrong token index";
        EXPECT_EQ(parsed_json.tokens[token_index].type, JSMN_STRING) << "Wrong token type returned";
        EXPECT_EQ(memcmp(transaction + parsed_json.tokens[token_index].start, "age", strlen("age")), 0)
                            << "Wrong key returned";
    }

    TEST(TxValidationTest, ObjectElementGet_string) {
        auto transaction = R"({"age":"36", "height":"185", "year":"1981", "month":"july"})";

        parsed_json_t parsed_json;
        JSON_PARSE(&parsed_json, transaction);

        uint16_t token_index;
        EXPECT_EQ(object_get_nth_value(&parsed_json, 0, 3, &token_index), parser_ok);
        EXPECT_EQ(token_index, 8) << "Wrong token index";
        EXPECT_EQ(parsed_json.tokens[token_index].type, JSMN_STRING) << "Wrong token type returned";
        EXPECT_EQ(memcmp(transaction + parsed_json.tokens[token_index].start, "july", strlen("july")), 0)
                            << "Wrong key returned";
    }

    TEST(TxValidationTest, ObjectElementGet_out_of_bounds_negative) {
        auto transaction = R"({"age":36, "height":185, "year":1981})";

        parsed_json_t parsed_json;
        JSON_PARSE(&parsed_json, transaction);

        uint16_t token_index;
        EXPECT_EQ(object_get_nth_key(&parsed_json, 0, -1, &token_index), parser_no_data)
                            << "Wrong token index, should be invalid";
    }

    TEST(TxValidationTest, ObjectElementGet_out_of_bounds) {
        auto transaction = R"({"age":36, "height":185, "year":1981})";

        parsed_json_t parsed_json;
        JSON_PARSE(&parsed_json, transaction);

        uint16_t token_index;
        EXPECT_EQ(object_get_nth_key(&parsed_json, 0, 5, &token_index), parser_no_data)
                            << "Wrong token index, should be invalid";
    }

    TEST(TxValidationTest, ObjectElementGet_array) {
        auto transaction = R"({ "ages":[36, 31, 10, 2],
                            "heights":[185, 164, 154, 132],
                            "years":[1981, 1985, 2008, 2016, 2022],
                            "months":["july", "august", "february", "july"]})";

        parsed_json_t parsed_json;
        JSON_PARSE(&parsed_json, transaction);

        uint16_t token_index;
        EXPECT_EQ(object_get_value(&parsed_json, 0, "years", &token_index), parser_ok);

        EXPECT_EQ(token_index, 14) << "Wrong token index";
        EXPECT_EQ(parsed_json.tokens[token_index].type, JSMN_ARRAY) << "Wrong token type returned";
        uint16_t number_elements;
        EXPECT_EQ(array_get_element_count(&parsed_json, token_index, &number_elements), parser_ok);
        EXPECT_EQ(number_elements, 5) << "Wrong number of array elements";
    }

    TEST(TxValidationTest, ObjectGetValueCorrectFormat) {
        auto transaction =
                R"({"account_number":"0","chain_id":"test-chain-1","fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";
        parsed_json_t parsed_json;
        JSON_PARSE(&parsed_json, transaction);

        uint16_t token_index;
        EXPECT_EQ(object_get_value(&parsed_json, 0, "alt_bytes", &token_index), parser_no_data)
                            << "Wrong token index"; // alt_bytes should not be found

        EXPECT_EQ(object_get_value(&parsed_json, 0, "account_number", &token_index), parser_ok);
        EXPECT_EQ(token_index, 2) << "Wrong token index"; // alt_bytes should not be found

        EXPECT_EQ(object_get_value(&parsed_json, 0, "chain_id", &token_index), parser_ok);
        EXPECT_EQ(token_index, 4) << "Wrong token index";

        EXPECT_EQ(object_get_value(&parsed_json, 0, "fee", &token_index), parser_ok);
        EXPECT_EQ(token_index, 6) << "Wrong token index";

        EXPECT_EQ(object_get_value(&parsed_json, 0, "msgs", &token_index), parser_ok);
        EXPECT_EQ(token_index, 19) << "Wrong token index";

        EXPECT_EQ(object_get_value(&parsed_json, 0, "sequence", &token_index), parser_ok);
        EXPECT_EQ(token_index, 46) << "Wrong token index";
    }
}
