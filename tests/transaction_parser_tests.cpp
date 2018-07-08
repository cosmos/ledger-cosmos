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
#include "lib/transaction_parser.h"
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <jsmn.h>
#include <lib/json_parser.h>

namespace {

// Test parsing real Cosmos transactions

void setup_context(
    parsed_json_t *parsed_json,
    unsigned short screen_size,
    const char *transaction) {
    parsing_context_t context;
    context.parsed_transaction = parsed_json;
    context.max_chars_per_key_line = screen_size;
    context.max_chars_per_value_line = screen_size;
    context.transaction = transaction;
    set_parsing_context(context);
    set_copy_delegate([](void *d, const void *s, size_t size) { memcpy(d, s, size); });
}

void EXPECT_EQ_STR(const char *str1, const char *str2, const char *errorMsg) {
    if (str1 != NULL & str2 != NULL) {
        EXPECT_TRUE(strcmp(str1, str2) == 0) << errorMsg << ", expected: " << str2 << ", received: " << str1;
    }
    else {
        FAIL() << "One of the strings is null";
    }
}


//TEST(TransactionParserTest, DisplayArbitraryItem_0) {
//
//    auto transaction =
//        R"({"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]}})";
//
//    parsed_json_t parsed_json;
//    json_parse(&parsed_json, transaction);
//
//    constexpr int screen_size = 100;
//    char key[screen_size] = "";
//    char value[screen_size] = "";
//    int chunk_index = 0;
//    setup_context(&parsed_json, screen_size, transaction);
//
//    display_arbitrary_item(
//        0,
//        key,
//        sizeof(key),
//        value,
//        sizeof(value),
//        2,
//        &chunk_index);
//
//    EXPECT_EQ_STR(key, "inputs/address", "Wrong key returned");
//    EXPECT_EQ_STR(value, "696E707574", "Wrong value returned");
//}
//
//TEST(TransactionParserTest, DisplayArbitraryItem_1) {
//
//    auto transaction =
//        R"({"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]}})";
//
//    parsed_json_t parsed_json;
//    json_parse(&parsed_json, transaction);
//
//    constexpr int screen_size = 100;
//    char key[screen_size] = "";
//    char value[screen_size] = "";
//    int chunk_index = 0;
//    setup_context(&parsed_json, screen_size, transaction);
//
//    display_arbitrary_item(
//        1,
//        key,
//        sizeof(key),
//        value,
//        sizeof(value),
//        2,
//        &chunk_index);
//
//    EXPECT_EQ_STR(key, "inputs/coins", "Wrong key returned");
//    EXPECT_EQ_STR(value, R"([{"amount":10,"denom":"atom"}])", "Wrong value returned");
//}
//
//TEST(TransactionParserTest, DisplayArbitraryItem_2) {
//
//    auto transaction =
//        R"({"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]}})";
//
//    parsed_json_t parsed_json;
//    json_parse(&parsed_json, transaction);
//
//    constexpr int screen_size = 100;
//    char key[screen_size] = "";
//    char value[screen_size] = "";
//    int chunk_index = 0;
//    setup_context(&parsed_json, screen_size, transaction);
//
//    display_arbitrary_item(
//        2,
//        key,
//        sizeof(key),
//        value,
//        sizeof(value),
//        2,
//        &chunk_index);
//
//    EXPECT_EQ_STR(key, "outputs/address", "Wrong key returned");
//    EXPECT_EQ_STR(value, "6F7574707574", "Wrong value returned");
//}
//
//TEST(TransactionParserTest, DisplayArbitraryItem_3) {
//
//    auto transaction =
//        R"({"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]}})";
//
//    parsed_json_t parsed_json;
//    json_parse(&parsed_json, transaction);
//
//    constexpr int screen_size = 100;
//    char key[screen_size] = "";
//    char value[screen_size] = "";
//    int requested_item_index = 3;
//    int chunk_index = 0;
//    setup_context(&parsed_json, screen_size, transaction);
//
//    int found_item_index = display_arbitrary_item(
//        requested_item_index,
//        key,
//        sizeof(key),
//        value,
//        sizeof(value),
//        2,
//        &chunk_index);
//
//    EXPECT_EQ_STR(key, "outputs/coins", "Wrong key returned");
//    EXPECT_EQ_STR(value, R"([{"amount":10,"denom":"atom"}])", "Wrong value returned");
//    EXPECT_EQ(found_item_index, requested_item_index) << "Returned wrong index";
//}
//
//TEST(TransactionParserTest, DisplayArbitraryItemCount) {
//
//    auto transaction =
//        R"({"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]}})";
//
//    parsed_json_t parsed_json;
//    json_parse(&parsed_json, transaction);
//
//    constexpr int screen_size = 100;
//
//    setup_context(&parsed_json, screen_size, transaction);
//
//    int found_item_index = display_get_arbitrary_items_count(2);
//    EXPECT_EQ(found_item_index, 4) << "Wrong number of displayable elements";
//}
//
//TEST(TransactionParserTest, ParseTransaction_Pages) {
//
//    auto transaction =
//        R"({"alt_bytes":null,"chain_id":"test-chain-1","fee_bytes":{"amount":[{"amount":5,"denom":"photon"}],"gas":10000},"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]},"sequence":1})";
//    parsed_json_t parsed_json;
//    json_parse(&parsed_json, transaction);
//
//    constexpr int screen_size = 100;
//    setup_context(&parsed_json, screen_size, transaction);
//    int pages = transaction_get_display_pages();
//    EXPECT_EQ(pages, 8) << "Wrong number of displayable pages";
//}

    TEST(TransactionParserTest, Transaction_Page_Count) {

        auto transaction =
                R"({"account_number":"0","chain_id":"test-chain-1","fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";

        parsed_json_t parsed_json;
        json_parse(&parsed_json, transaction);

        constexpr int screen_size = 100;
        setup_context(&parsed_json, screen_size, transaction);

        char key[screen_size];
        char value[screen_size];
        int chunk_index = 0;

        EXPECT_EQ(9, transaction_get_display_pages()) << "Wrong number of pages";
    }

    TEST(TransactionParserTest, Transaction_Page_Count_MultipleMsgs) {

        auto transaction =
                R"({"account_number":"0","chain_id":"test-chain-1","fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]},{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]},{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]},{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";

        parsed_json_t parsed_json;
        json_parse(&parsed_json, transaction);

        constexpr int screen_size = 100;
        setup_context(&parsed_json, screen_size, transaction);

        char key[screen_size];
        char value[screen_size];
        int chunk_index = 0;

        EXPECT_EQ(21, transaction_get_display_pages()) << "Wrong number of pages";
    }

    TEST(TransactionParserTest, ParseTransaction_Page_1) {

        auto transaction =
                R"({"account_number":"0","chain_id":"test-chain-1","fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";

        parsed_json_t parsed_json;
        json_parse(&parsed_json, transaction);

        constexpr int screen_size = 100;
        setup_context(&parsed_json, screen_size, transaction);

        char key[screen_size];
        char value[screen_size];
        int chunk_index = 0;

        // Get key/value strings for the 1st page
        transaction_get_display_key_value(
                key,
                sizeof(key),
                value,
                sizeof(value),
                0,
                &chunk_index);

        EXPECT_EQ_STR(key, "chain_id", "Wrong key");
        EXPECT_EQ_STR(value, "test-chain-1", "Wrong value");
    }

    TEST(TransactionParserTest, ParseTransaction_Page_2) {

        auto transaction =
                R"({"account_number":"0","chain_id":"test-chain-1","fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";

        parsed_json_t parsed_json;
        json_parse(&parsed_json, transaction);

        constexpr int screen_size = 100;
        setup_context(&parsed_json, screen_size, transaction);

        char key[screen_size];
        char value[screen_size];
        int chunk_index = 0;

        // Get key/value strings for the 2nd page
        transaction_get_display_key_value(
                key,
                sizeof(key),
                value,
                sizeof(value),
                1,
                &chunk_index);

        EXPECT_EQ_STR(key, "account_number", "Wrong key");
        EXPECT_EQ_STR(value, "0", "Wrong value");
    }

    TEST(TransactionParserTest, ParseTransaction_Page_3) {

        auto transaction =
                R"({"account_number":"0","chain_id":"test-chain-1","fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";

        parsed_json_t parsed_json;
        json_parse(&parsed_json, transaction);

        constexpr int screen_size = 100;
        setup_context(&parsed_json, screen_size, transaction);

        char key[screen_size];
        char value[screen_size];
        int chunk_index = 0;

        // Get key/value strings for the 3rd page
        transaction_get_display_key_value(
                key,
                sizeof(key),
                value,
                sizeof(value),
                2,
                &chunk_index);

        EXPECT_EQ_STR(key, "sequence", "Wrong key");
        EXPECT_EQ_STR(value, "1", "Wrong value");
    }

    TEST(TransactionParserTest, ParseTransaction_Page_4) {

        auto transaction =
                R"({"account_number":"0","chain_id":"test-chain-1","fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";

        parsed_json_t parsed_json;
        json_parse(&parsed_json, transaction);

        constexpr int screen_size = 100;
        setup_context(&parsed_json, screen_size, transaction);

        char key[screen_size];
        char value[screen_size];
        int chunk_index = 0;

        // Get key/value strings for the 4th page
        transaction_get_display_key_value(
                key,
                sizeof(key),
                value,
                sizeof(value),
                3,
                &chunk_index);


        EXPECT_EQ_STR(key, "fee", "Wrong key");
        EXPECT_EQ_STR(value, "{\"amount\":[{\"amount\":\"5\",\"denom\":\"photon\"}],\"gas\":\"10000\"}", "Wrong value");
    }

    TEST(TransactionParserTest, ParseTransaction_Page_5) {

        auto transaction =
                R"({"account_number":"0","chain_id":"test-chain-1","fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";

        parsed_json_t parsed_json;
        json_parse(&parsed_json, transaction);

        constexpr int screen_size = 100;
        setup_context(&parsed_json, screen_size, transaction);

        char key[screen_size];
        char value[screen_size];
        int chunk_index = 0;

        // Get key/value strings for the 5th page
        transaction_get_display_key_value(
                key,
                sizeof(key),
                value,
                sizeof(value),
                4,
                &chunk_index);

        EXPECT_EQ_STR(key, "memo", "Wrong key");
        EXPECT_EQ_STR(value, "testmemo", "Wrong value");
    }

TEST(TransactionParserTest, ParseTransaction_Page_6) {

    auto transaction =
            R"({"account_number":"0","chain_id":"test-chain-1","fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";
    parsed_json_t parsed_json;
    json_parse(&parsed_json, transaction);

    constexpr int screen_size = 100;
    setup_context(&parsed_json, screen_size, transaction);

    char key[screen_size];
    char value[screen_size];
    int chunk_index = 0;

    // Get key/value strings for the 6th page, which
    // is the first page of recursive parsing of msg
    transaction_get_display_key_value(
        key,
        sizeof(key),
        value,
        sizeof(value),
        5,
        &chunk_index);

    EXPECT_EQ_STR(key, "msgs_1/inputs/address", "Wrong key");
    EXPECT_EQ_STR(value, "cosmosaccaddr1d9h8qat5e4ehc5", "Wrong value");
}

TEST(TransactionParserTest, ParseTransaction_Page_7) {

    auto transaction =
            R"({"account_number":"0","chain_id":"test-chain-1","fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";

    parsed_json_t parsed_json;
    json_parse(&parsed_json, transaction);

    constexpr int screen_size = 50;
    setup_context(&parsed_json, screen_size, transaction);

    char key[100];
    char value[100];
    int chunk_index = 0;
    // Get key/value strings for the 7th page, which
    // is the second page of recursive parsing of msg
    transaction_get_display_key_value(
        key,
        sizeof(key),
        value,
        sizeof(value),
        6,
        &chunk_index);

    EXPECT_EQ_STR(key, "msgs_1/inputs/coins", "Wrong key");
    EXPECT_EQ_STR(value, "[{\"amount\":\"10\",\"denom\":\"atom\"}]", "Wrong value");
}

//TEST(TransactionParserTest, ParseTransaction_Page_7) {
//
//    auto transaction =
//        R"({"alt_bytes":null,"chain_id":"test-chain-1","fee_bytes":{"amount":[{"amount":5,"denom":"photon"}],"gas":10000},"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]},"sequence":1})";
//    parsed_json_t parsed_json;
//    json_parse(&parsed_json, transaction);
//
//    constexpr int screen_size = 100;
//    setup_context(&parsed_json, screen_size, transaction);
//
//    char key[screen_size];
//    char value[screen_size];
//    int chunk_index = 0;
//    transaction_get_display_key_value(
//        key, sizeof(key),
//        value, sizeof(value),
//        6, &chunk_index);
//
//    EXPECT_EQ_STR(key, "msg_bytes/outputs/coins", "Wrong key");
//    EXPECT_EQ_STR(value, R"([{"amount":10,"denom":"atom"}])", "Wrong value");
//}
//
//TEST(TransactionParserTest, ParseTransaction_Page_8) {
//
//    auto transaction =
//        R"({"alt_bytes":null,"chain_id":"test-chain-1","fee_bytes":{"amount":[{"amount":5,"denom":"photon"}],"gas":10000},"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]},"sequence":1})";
//    parsed_json_t parsed_json;
//    json_parse(&parsed_json, transaction);
//
//    constexpr int screen_size = 100;
//    setup_context(&parsed_json, screen_size, transaction);
//
//    char key[screen_size];
//    char value[screen_size];
//    int chunk_index = 0;
//    transaction_get_display_key_value(
//        key, sizeof(key),
//        value, sizeof(value),
//        7, &chunk_index);
//
//    EXPECT_EQ_STR(key, "alt_bytes", "Wrong key");
//    EXPECT_EQ_STR(value, "null", "Wrong value");
//}
//
//TEST(TransactionParserTest, ParseTransaction_ValueFitsScreen) {
//
//    auto transaction =
//        R"({"alt_bytes":null,"chain_id":"test-chain-1","fee_bytes":"Four","msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]},"sequence":1})";
//    parsed_json_t parsed_json;
//    json_parse(&parsed_json, transaction);
//
//    constexpr int screen_size = 5;
//    setup_context(&parsed_json, screen_size, transaction);
//
//    char key[10];
//    char value[screen_size];
//    int chunk_index = 0;
//    transaction_get_display_key_value(
//        key, sizeof(key),
//        value, sizeof(value),
//        2, &chunk_index);
//
//    EXPECT_EQ(chunk_index, 1) << "Wrong number of chunks";
//    EXPECT_EQ_STR(value, "Four", "Wrong value");
//}
//
//TEST(TransactionParserTest, ParseTransaction_ValueCharacterShortToFitScreen) {
//
//    auto transaction =
//        R"({"alt_bytes":null,"chain_id":"test-chain-1","fee_bytes":"Fourt","msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]},"sequence":1})";
//    parsed_json_t parsed_json;
//    json_parse(&parsed_json, transaction);
//
//    constexpr int screen_size = 5;
//    setup_context(&parsed_json, screen_size, transaction);
//
//    char key[10];
//    char value[screen_size];
//    int chunk_index = 0;
//    transaction_get_display_key_value(
//        key, sizeof(key),
//        value, sizeof(value),
//        2, &chunk_index);
//
//    EXPECT_EQ(chunk_index, 2) << "Wrong number of chunks";
//    // Because string is null terminated there was not enough room for 't'
//    EXPECT_EQ_STR(value, "Four", "Wrong value");
//
//    chunk_index = 1;
//    transaction_get_display_key_value(
//        key, sizeof(key),
//        value, sizeof(value),
//        2, &chunk_index);
//
//    EXPECT_EQ(chunk_index, 2) << "Wrong number of chunks";
//    // Because string is null terminated there was not enough room for 't'
//    EXPECT_EQ_STR(value, "t", "Wrong value");
//}
//
//TEST(TransactionParserTest, ParseTransaction_VeryLongValue) {
//
//    auto transaction =
//        R"({"alt_bytes":null,"chain_id":"test-chain-1","fee_bytes":{"amount":[{"amount":5,"denom":"photon"}],"gas":10000},"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":"LONGJUMPLIFELOVEDOVE"}]},"sequence":1})";
//    parsed_json_t parsed_json;
//    json_parse(&parsed_json, transaction);
//
//    constexpr int screen_size = 5;
//    setup_context(&parsed_json, screen_size, transaction);
//
//    char key[32];
//    char value[screen_size];
//
//    // String: LONGJUMPLIFELOVEDOVE
//
//    int chunk_index = 0;
//    transaction_get_display_key_value(
//        key, sizeof(key),
//        value, sizeof(value),
//        6, &chunk_index);
//
//    EXPECT_EQ(chunk_index, 5) << "Wrong number of chunks";
//    EXPECT_EQ_STR(value, "LONG", "Wrong value");
//
//    chunk_index = 1;
//    transaction_get_display_key_value(
//        key, sizeof(key),
//        value, sizeof(value),
//        6, &chunk_index);
//
//    EXPECT_EQ(chunk_index, 5) << "Wrong number of chunks";
//    EXPECT_EQ_STR(value, "JUMP", "Wrong value");
//
//    chunk_index = 2;
//    transaction_get_display_key_value(
//        key, sizeof(key),
//        value, sizeof(value),
//        6, &chunk_index);
//
//    EXPECT_EQ(chunk_index, 5) << "Wrong number of chunks";
//    EXPECT_EQ_STR(value, "LIFE", "Wrong value");
//
//    chunk_index = 3;
//    transaction_get_display_key_value(
//        key, sizeof(key),
//        value, sizeof(value),
//        6, &chunk_index);
//
//    EXPECT_EQ(chunk_index, 5) << "Wrong number of chunks";
//    EXPECT_EQ_STR(value, "LOVE", "Wrong value");
//
//    chunk_index = 4;
//    transaction_get_display_key_value(
//        key, sizeof(key),
//        value, sizeof(value),
//        6, &chunk_index);
//
//    EXPECT_EQ(chunk_index, 5) << "Wrong number of chunks";
//    EXPECT_EQ_STR(value, "DOVE", "Wrong value");
//}
//
//TEST(TransactionParserTest, ParseTransaction_OutOfBounds) {
//
//    auto transaction =
//        R"({"alt_bytes":null,"chain_id":"test-chain-1","fee_bytes":{"amount":[{"amount":5,"denom":"photon"}],"gas":10000},"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":"LONGJUMPLIFELOVEDOVE"}]},"sequence":1})";
//    parsed_json_t parsed_json;
//    json_parse(&parsed_json, transaction);
//
//    constexpr int screen_size = 5;
//    setup_context(&parsed_json, screen_size, transaction);
//
//    char key[32];
//    char value[screen_size];
//
//    // String: LONGJUMPLIFELOVEDOVE
//
//    int chunk_index = -1;
//    transaction_get_display_key_value(
//        key, sizeof(key),
//        value, sizeof(value),
//        6, &chunk_index);
//
//    EXPECT_EQ(chunk_index, 5) << "Wrong number of chunks";
//    EXPECT_EQ_STR(value, "", "Wrong value");
//
//    chunk_index = 10;
//    transaction_get_display_key_value(
//        key, sizeof(key),
//        value, sizeof(value),
//        6, &chunk_index);
//
//    EXPECT_EQ(chunk_index, 5) << "Wrong number of chunks";
//    EXPECT_EQ_STR(value, "", "Wrong value");
//}
//
//TEST(TransactionParserTest, TransactionJsonValidation_CorrectFormat) {
//
//    auto transaction = R"({"alt_bytes":null,"chain_id":"test-chain-1","fee_bytes":{"amount":[{"amount":5,"denom":"photon"}],"gas":10000},"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]},"sequence":1})";
//
//    parsed_json_t parsed_transaction;
//    json_parse(&parsed_transaction, transaction);
//
//    const char* error_msg = json_validate(&parsed_transaction, transaction);
//    EXPECT_TRUE(error_msg == NULL) << "Validation failed, error: " << error_msg;
//}
//
//TEST(TransactionParserTest, TransactionJsonValidation_MissingAltBytes) {
//
//    auto transaction = R"({"chain_id":"test-chain-1","fee_bytes":{"amount":[{"amount":5,"denom":"photon"}],"gas":10000},"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]},"sequence":1})";
//
//    parsed_json_t parsed_transaction;
//    json_parse(&parsed_transaction, transaction);
//
//    const char* error_msg = json_validate(&parsed_transaction, transaction);
//    EXPECT_EQ_STR(error_msg, "Missing alt_bytes", "Validation should fail because alt_bytes are missing");
//}
//
//TEST(TransactionParserTest, TransactionJsonValidation_MissingChainId) {
//
//    auto transaction = R"({"alt_bytes":null,"fee_bytes":{"amount":[{"amount":5,"denom":"photon"}],"gas":10000},"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]},"sequence":1})";
//
//    parsed_json_t parsed_transaction;
//    json_parse(&parsed_transaction, transaction);
//
//    const char* error_msg = json_validate(&parsed_transaction, transaction);
//    EXPECT_EQ_STR(error_msg, "Missing chain_id", "Validation should fail because chain_id is missing");
//}
//
//TEST(TransactionParserTest, TransactionJsonValidation_MissingFeeBytes) {
//
//    auto transaction = R"({"alt_bytes":null,"chain_id":"test-chain-1","msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]},"sequence":1})";
//
//    parsed_json_t parsed_transaction;
//    json_parse(&parsed_transaction, transaction);
//
//    const char* error_msg = json_validate(&parsed_transaction, transaction);
//    EXPECT_EQ_STR(error_msg, "Missing fee_bytes", "Validation should fail because fee_bytes are missing");
//}
//
//TEST(TransactionParserTest, TransactionJsonValidation_MissingMsgBytes) {
//
//    auto transaction = R"({"alt_bytes":null,"chain_id":"test-chain-1","fee_bytes":{"amount":[{"amount":5,"denom":"photon"}],"gas":10000},"sequence":1})";
//
//    parsed_json_t parsed_transaction;
//    json_parse(&parsed_transaction, transaction);
//
//    const char* error_msg = json_validate(&parsed_transaction, transaction);
//    EXPECT_EQ_STR(error_msg, "Missing msg_bytes", "Validation should fail because msg_bytes are missing");
//}
//
//TEST(TransactionParserTest, TransactionJsonValidation_MissingSequence) {
//
//    auto transaction = R"({"alt_bytes":null,"chain_id":"test-chain-1","fee_bytes":{"amount":[{"amount":5,"denom":"photon"}],"gas":10000},"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]}})";
//
//    parsed_json_t parsed_transaction;
//    json_parse(&parsed_transaction, transaction);
//
//    const char* error_msg = json_validate(&parsed_transaction, transaction);
//    EXPECT_EQ_STR(error_msg, "Missing sequence", "Validation should fail because sequence is missing");
//}
//
//TEST(TransactionParserTest, TransactionJsonValidation_Spaces_InTheMiddle) {
//
//    auto transaction = R"({"alt_bytes":null,"chain_id" :"test-chain-1","fee_bytes":{"amount":[{"amount":5,"denom":"photon"}],"gas":10000},"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]},"sequence":1})";
//
//    parsed_json_t parsed_transaction;
//    json_parse(&parsed_transaction, transaction);
//
//    const char* error_msg = json_validate(&parsed_transaction, transaction);
//    EXPECT_EQ_STR(error_msg, "Contains whitespace in the corpus", "Validation should fail because contains whitespace in the corpus");
//}
//TEST(TransactionParserTest, TransactionJsonValidation_Spaces_AtTheFront) {
//
//    auto transaction = R"({\t"alt_bytes":null,"chain_id" :"test-chain-1","fee_bytes":{"amount":[{"amount":5,"denom":"photon"}],"gas":10000},"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]},"sequence":1})";
//
//    parsed_json_t parsed_transaction;
//    json_parse(&parsed_transaction, transaction);
//
//    const char* error_msg = json_validate(&parsed_transaction, transaction);
//    EXPECT_EQ_STR(error_msg, "Contains whitespace in the corpus", "Validation should fail because contains whitespace in the corpus");
//}
//TEST(TransactionParserTest, TransactionJsonValidation_Spaces_AtTheEnd) {
//
//    auto transaction = R"({"alt_bytes":null,"chain_id" :"test-chain-1","fee_bytes":{"amount":[{"amount":5,"denom":"photon"}],"gas":10000},"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]},"sequence":1\r})";
//
//    parsed_json_t parsed_transaction;
//    json_parse(&parsed_transaction, transaction);
//
//    const char* error_msg = json_validate(&parsed_transaction, transaction);
//    EXPECT_EQ_STR(error_msg, "Contains whitespace in the corpus", "Validation should fail because contains whitespace in the corpus");
//}
//TEST(TransactionParserTest, TransactionJsonValidation_Spaces_Lots) {
//
//    auto transaction = R"({"alt_bytes":null,"chain_id" :"test-chain-1",\t\t"fee_bytes":     {"amount":[{"amount":5,"denom":"photon"}],"gas":10000},"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]},"sequence":1})";
//
//    parsed_json_t parsed_transaction;
//    json_parse(&parsed_transaction, transaction);
//
//    const char* error_msg = json_validate(&parsed_transaction, transaction);
//    EXPECT_EQ_STR(error_msg, "Contains whitespace in the corpus", "Validation should fail because contains whitespace in the corpus");
//}
//
//TEST(TransactionParserTest, TransactionJsonValidation_AllowSpacesInString) {
//
//    auto transaction = R"({"alt_bytes\t":null,"chain_id\r":"test-chain-1\n","fee_bytes":{"amount":[{"amount":5,"denom":"photon"}],"gas":10000},"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount\n\n\n\n\n":10,"denom":"atom"}]}]},"sequence\t\t\t\t":1})";
//
//    parsed_json_t parsed_transaction;
//    json_parse(&parsed_transaction, transaction);
//
//    const char* error_msg = json_validate(&parsed_transaction, transaction);
//    EXPECT_TRUE(error_msg == NULL) << "Validation failed, error: " << error_msg;
//}
//
//TEST(TransactionParserTest, TransactionJsonValidation_SortedDictionary) {
//
//    auto transaction = R"({"alt_bytes\t":null,"chain_id\r":"test-chain-1\n","fee_bytes":{"amount":[{"amount":5,"denom":"photon"}],"gas":10000},"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount\n\n\n\n\n":10,"denom":"atom"}]}]},"sequence\t\t\t\t":1})";
//
//    parsed_json_t parsed_transaction;
//    json_parse(&parsed_transaction, transaction);
//
//    const char* error_msg = json_validate(&parsed_transaction, transaction);
//    EXPECT_TRUE(error_msg == NULL) << "Validation failed, error: " << error_msg;
//}
//
//#ifndef DISABLE_KEY_SORTING_JSON_VALIDATION
//TEST(TransactionParserTest, TransactionJsonValidation_NotSortedDictionary_FirstElement) {
//
//    auto transaction = R"({"chain_id":"test-chain-1","alt_bytes":null,"fee_bytes":{"amount":[{"amount":5,"denom":"photon"}],"gas":10000},"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]},"sequence":1})";
//
//    parsed_json_t parsed_transaction;
//    json_parse(&parsed_transaction, transaction);
//
//    const char* error_msg = json_validate(&parsed_transaction, transaction);
//    EXPECT_EQ_STR(error_msg, "Dictionaries are not sorted", "Validation should fail because dictionaries are not sorted");
//}
//
//TEST(TransactionParserTest, TransactionJsonValidation_NotSortedDictionary_MiddleElement) {
//
//    auto transaction = R"({"alt_bytes":null,"chain_id":"test-chain-1","fee_bytes":{"amount":[{"denom":"photon","amount":5}],"gas":10000},"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]},"sequence":1})";
//
//    parsed_json_t parsed_transaction;
//    json_parse(&parsed_transaction, transaction);
//
//    const char *error_msg = json_validate(&parsed_transaction, transaction);
//    EXPECT_EQ_STR(error_msg, "Dictionaries are not sorted",
//                  "Validation should fail because dictionaries are not sorted");
//}
//
//TEST(TransactionParserTest, TransactionJsonValidation_NotSortedDictionary_LastElement) {
//
//    auto transaction = R"({"alt_bytes":null,"chain_id":"test-chain-1","fee_bytes":{"amount":[{"amount":5,"denom":"photon"}],"gas":10000},"sequence":1,"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]}})";
//
//    parsed_json_t parsed_transaction;
//    json_parse(&parsed_transaction, transaction);
//
//    const char *error_msg = json_validate(&parsed_transaction, transaction);
//    EXPECT_EQ_STR(error_msg, "Dictionaries are not sorted",
//                  "Validation should fail because dictionaries are not sorted");
//}
//#endif
//
//
//// This json has been taken directly from goclient which uses cosmos to serialize a simple transaction
//// This test is currently failing the validation.
//// We are reviewing the validation code and cosmos serialization to find the culprit.
//
//TEST(TransactionParserTest, TransactionJsonValidation_CosmosExample) {
//
//    auto transaction = R"({"chain_id":"test-chain-1","sequences":[1],"fee_bytes":{"amount":[{"denom":"photon","amount":5}],"gas":10000},"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"denom":"atom","amount":10}]}],"outputs":[{"address":"6F7574707574","coins":[{"denom":"atom","amount":10}]}]},"alt_bytes":null})";
//
//    parsed_json_t parsed_transaction;
//    json_parse(&parsed_transaction, transaction);
//
//    const char *error_msg = json_validate(&parsed_transaction, transaction);
//    EXPECT_TRUE(error_msg == NULL) << "Validation failed, error: " << error_msg;
//}
}