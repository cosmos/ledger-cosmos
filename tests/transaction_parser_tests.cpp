/*******************************************************************************
*   (c) 2016 Ledger
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

    TEST(TransactionParserTest, correct_format) {

        auto transaction = R"({"alt_bytes":null,"chain_id":"test-chain-1","fee_bytes":{"amount":[{"amount":5,"denom":"photon"}],"gas":10000},"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]},"sequences":[1]})";
        char errorMsg[20];
        int result = json_validate(transaction, errorMsg, sizeof(errorMsg));
        EXPECT_TRUE(result == 0) << "Validation failed, error: " << errorMsg;
    }

    TEST(TransactionParserTest, incorrect_format_missing_alt_bytes) {

        auto transaction = R"({"chain_id":"test-chain-1","fee_bytes":{"amount":[{"amount":5,"denom":"photon"}],"gas":10000},"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]},"sequences":[1]})";
        char errorMsg[20];
        int result = json_validate(transaction, errorMsg, sizeof(errorMsg));
        EXPECT_TRUE(result == -1) << "Validation should fail, alt_bytes are missing";
    }

    TEST(TransactionParserTest, incorrect_format_missing_chain_id) {

        auto transaction = R"({"alt_bytes":null,"fee_bytes":{"amount":[{"amount":5,"denom":"photon"}],"gas":10000},"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]},"sequences":[1]})";
        char errorMsg[20];
        int result = json_validate(transaction, errorMsg, sizeof(errorMsg));
        EXPECT_TRUE(result == -1) << "Validation should fail, chain_id is missing";
    }

    TEST(TransactionParserTest, incorrect_format_missing_fee_bytes) {

        auto transaction = R"({"alt_bytes":null,"chain_id":"test-chain-1","msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]},"sequences":[1]})";
        char errorMsg[20];
        int result = json_validate(transaction, errorMsg, sizeof(errorMsg));
        EXPECT_TRUE(result == -1) << "Validation should fail, fee_bytes are missing";
    }

    TEST(TransactionParserTest, incorrect_format_missing_msg_bytes) {

        auto transaction = R"({"alt_bytes":null,"chain_id":"test-chain-1","fee_bytes":{"amount":[{"amount":5,"denom":"photon"}],"gas":10000},"sequences":[1]})";
        char errorMsg[20];
        int result = json_validate(transaction, errorMsg, sizeof(errorMsg));
        EXPECT_TRUE(result == -1) << "Validation should fail, msg_bytes are missing";
    }

    TEST(TransactionParserTest, incorrect_format_missing_sequence) {

        auto transaction = R"({"alt_bytes":null,"chain_id":"test-chain-1","fee_bytes":{"amount":[{"amount":5,"denom":"photon"}],"gas":10000},"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]}})";
        char errorMsg[20];
        int result = json_validate(transaction, errorMsg, sizeof(errorMsg));
        EXPECT_TRUE(result == -1) << "Validation should fail, sequence is missing";
    }

    int get_token_valid_message(const char* name, int parent_token_index)
    {
        auto transaction = R"({"alt_bytes":null,"chain_id":"test-chain-1","fee_bytes":{"amount":[{"amount":5,"denom":"photon"}],"gas":10000},"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]},"sequences":[1]})";
        char errorMsg[20];

        parsed_json_t parsed_json;
        json_parse(&parsed_json, transaction);

        int result = json_get_child_token(
                transaction,
                &parsed_json,
                name,
                parent_token_index);
        return result;
    }

    int get_token_valid_message(int index, int parent_token_index)
    {
        auto transaction = R"({"alt_bytes":null,"chain_id":"test-chain-1","fee_bytes":{"amount":[{"amount":5,"denom":"photon"}],"gas":10000},"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]},"sequences":[1]})";
        char errorMsg[20];

        parsed_json_t parsed_json;
        json_parse(&parsed_json, transaction);

        int result = json_get_child_token_by_index(
                transaction,
                &parsed_json,
                index,
                parent_token_index);
        return result;
    }

    TEST(TransactionParserTest, get_token_alt_bytes) {
        auto result = get_token_valid_message("alt_bytes", 0);
        EXPECT_EQ(result, 1) << "Wrong token index, expected: " << 1 << ", received: " << result;
    }

    TEST(TransactionParserTest, get_token_alt_bytes_index) {
        auto result = get_token_valid_message(0, 0);
        EXPECT_EQ(result, 1) << "Wrong token index, expected: " << 1 << ", received: " << result;
    }

    TEST(TransactionParserTest, get_token_chain_id) {
        auto result = get_token_valid_message("chain_id", 0);
        EXPECT_EQ(result, 3) << "Wrong token index, expected: " << 3 << ", received: " << result;
    }

    TEST(TransactionParserTest, get_token_chain_id_index) {
        auto result = get_token_valid_message(1, 0);
        EXPECT_EQ(result, 3) << "Wrong token index, expected: " << 3 << ", received: " << result;
    }

    TEST(TransactionParserTest, get_token_fee_bytes) {
        auto result = get_token_valid_message("fee_bytes", 0);
        EXPECT_EQ(result, 5) << "Wrong token index, expected: " << 5 << ", received: " << result;
    }

    TEST(TransactionParserTest, get_token_fee_bytes_index) {
        auto result = get_token_valid_message(2, 0);
        EXPECT_EQ(result, 5) << "Wrong token index, expected: " << 5 << ", received: " << result;
    }

    TEST(TransactionParserTest, get_token_msg_bytes) {
        auto result = get_token_valid_message("msg_bytes", 0);
        EXPECT_EQ(result, 14) << "Wrong token index, expected: " << 14 << ", received: " << result;
    }

    TEST(TransactionParserTest, get_token_msg_bytes_index) {
        auto result = get_token_valid_message(3, 0);
        EXPECT_EQ(result, 14) << "Wrong token index, expected: " << 14 << ", received: " << result;
    }

    TEST(TransactionParserTest, get_token_sequences) {
        auto result = get_token_valid_message("sequences", 0);
        EXPECT_EQ(result, 32) << "Wrong token index, expected: " << 32 << ", received: " << result;
    }

    TEST(TransactionParserTest, get_token_sequences_index) {
        auto result = get_token_valid_message(4, 0);
        EXPECT_EQ(result, 32) << "Wrong token index, expected: " << 32 << ", received: " << result;
    }

    TEST(TransactionParserTest, get_token_alt_bytes_wrong_level) {
        auto result = get_token_valid_message("alt_bytes", 2);
        EXPECT_EQ(result, -1) << "Wrong token index, expected: " << 0 << ", received: " << result;
    }
    TEST(TransactionParserTest, get_token_chain_id_wrong_level) {
        auto result = get_token_valid_message("chain_id", 3);
        EXPECT_EQ(result, -1) << "Wrong token index, expected: " << 0 << ", received: " << result;
    }
    TEST(TransactionParserTest, get_token_fee_bytes_wrong_level) {
        auto result = get_token_valid_message("fee_bytes", 10);
        EXPECT_EQ(result, -1) << "Wrong token index, expected: " << 0 << ", received: " << result;
    }
    TEST(TransactionParserTest, get_token_msg_bytes_wrong_level) {
        auto result = get_token_valid_message("msg_bytes", 4);
        EXPECT_EQ(result, -1) << "Wrong token index, expected: " << 0 << ", received: " << result;
    }
    TEST(TransactionParserTest, get_token_sequences_wrong_level) {
        auto result = get_token_valid_message("sequences", -1);
        EXPECT_EQ(result, -1) << "Wrong token index, expected: " << 0 << ", received: " << result;
    }

    TEST(TransactionParserTest, read_chain_id) {
        auto transaction = R"({"alt_bytes":null,"chain_id":"test-chain-1","fee_bytes":{"amount":[{"amount":5,"denom":"photon"}],"gas":10000},"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]},"sequences":[1]})";
        char errorMsg[20];

        parsed_json_t parsed_json;
        json_parse(&parsed_json, transaction);
        int token_index = json_get_child_token(
                transaction,
                &parsed_json,
                "chain_id",
                0);

        char buffer[20];
        // Read name
        json_read_token(transaction, &parsed_json, token_index, buffer, sizeof(buffer), 0);
        auto cmp = "chain_id";
        EXPECT_TRUE(strcmp(buffer, cmp) == 0) << "Incorrect name, expected:" << cmp << ", received:" << buffer;

        // Read value
        json_read_token(transaction, &parsed_json, token_index+1, buffer, sizeof(buffer), 0);
        cmp = "test-chain-1";
        EXPECT_TRUE(strcmp(buffer, cmp) == 0) << "Incorrect name, expected:" << cmp << ", received:" << buffer;
    }

    TEST(TransactionParserTest, read_fee_offset_0) {
        auto transaction = R"({"alt_bytes":null,"chain_id":"test-chain-1","fee_bytes":{"amount":[{"amount":5,"denom":"photon"}],"gas":10000},"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]},"sequences":[1]})";
        char errorMsg[20];

        parsed_json_t parsed_json;
        json_parse(&parsed_json, transaction);
        int token_index = json_get_child_token(
                transaction,
                &parsed_json,
                "fee_bytes",
                0);

        char buffer[20];
        json_read_token(transaction, &parsed_json, token_index+1, buffer, sizeof(buffer), 0);
        auto cmp = R"({"amount":[{"amount")";
        EXPECT_TRUE(strcmp(buffer, cmp) == 0) << "Incorrect value of fee, expected:" << cmp << ", received:" << buffer;
    }

    TEST(TransactionParserTest, read_fee_offset_20) {
        auto transaction = R"({"alt_bytes":null,"chain_id":"test-chain-1","fee_bytes":{"amount":[{"amount":5,"denom":"photon"}],"gas":10000},"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]},"sequences":[1]})";
        char errorMsg[20];

        parsed_json_t parsed_json;
        json_parse(&parsed_json, transaction);
        int token_index = json_get_child_token(
                transaction,
                &parsed_json,
                "fee_bytes",
                0);

        char buffer[20];
        json_read_token(transaction, &parsed_json, token_index+1, buffer, sizeof(buffer), 20);
        auto cmp = R"(":5,"denom":"photon)";
        EXPECT_TRUE(strcmp(buffer, cmp) == 0) << "Incorrect value of fee, expected:" << cmp << ", received:" << buffer;
    }

    TEST(TransactionParserTest, read_msg_bytes_item_1_level_0) {
        auto transaction = R"({"alt_bytes":null,"chain_id":"test-chain-1","fee_bytes":{"amount":[{"amount":5,"denom":"photon"}],"gas":10000},"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]},"sequences":[1]})";
        char errorMsg[20];

        parsed_json_t parsed_json;
        json_parse(&parsed_json, transaction);

        // First find the msg_bytes parent token, level 0
        int token_index = json_get_child_token(
                transaction,
                &parsed_json,
                "msg_bytes",
                0);

        // Because token_index+1 is object
        int child_token_level_0 = json_get_child_token_by_index(
                transaction,
                &parsed_json,
                0,
                token_index+1);

        char buffer[100];
        json_read_token(transaction, &parsed_json, child_token_level_0, buffer, sizeof(buffer), 0);
        auto cmp = "inputs";
        EXPECT_TRUE(strcmp(buffer, cmp) == 0) << "Incorrect name, expected:" << cmp << ", received:" << buffer;

        // Because child_token_level_0+1 is object
        int child_token_level_1 = json_get_child_token_by_index(
                transaction,
                &parsed_json,
                0,
                child_token_level_0+1);

        json_read_token(transaction, &parsed_json, child_token_level_1, buffer, sizeof(buffer), 0);
        cmp = "address";
        EXPECT_TRUE(strcmp(buffer, cmp) == 0) << "Incorrect name, expected:" << cmp << ", received:" << buffer;

        // Because child_token_level_1+1 is string
        json_read_token(transaction, &parsed_json, child_token_level_1+1, buffer, sizeof(buffer), 0);
        cmp = "696E707574";
        EXPECT_TRUE(strcmp(buffer, cmp) == 0) << "Incorrect name, expected:" << cmp << ", received:" << buffer;
    }

    TEST(TransactionParserTest, read_msg_bytes_item_2_level_1) {
        auto transaction = R"({"alt_bytes":null,"chain_id":"test-chain-1","fee_bytes":{"amount":[{"amount":5,"denom":"photon"}],"gas":10000},"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]},"sequences":[1]})";
        char errorMsg[20];

        parsed_json_t parsed_json;
        json_parse(&parsed_json, transaction);

        // First find the msg_bytes parent token, level 0
        int token_index = json_get_child_token(
                transaction,
                &parsed_json,
                "msg_bytes",
                0);

        // Because token_index+1 is object
        int child_token_level_0 = json_get_child_token_by_index(
                transaction,
                &parsed_json,
                0,
                token_index+1);

        char buffer[100];
        json_read_token(transaction, &parsed_json, child_token_level_0, buffer, sizeof(buffer), 0);
        auto cmp = "inputs";
        EXPECT_TRUE(strcmp(buffer, cmp) == 0) << "Incorrect name, expected:" << cmp << ", received:" << buffer;

        // Because child_token_level_0+1 is object
        int child_token_level_1 = json_get_child_token_by_index(
                transaction,
                &parsed_json,
                1,
                child_token_level_0+1);

        json_read_token(transaction, &parsed_json, child_token_level_1, buffer, sizeof(buffer), 0);
        cmp = "coins";
        EXPECT_TRUE(strcmp(buffer, cmp) == 0) << "Incorrect name, expected:" << cmp << ", received:" << buffer;

        // Although child_token_level_1+1 is object, we have restriction to stringify level 2
        json_read_token(transaction, &parsed_json, child_token_level_1+1, buffer, sizeof(buffer), 0);
        cmp = R"([{\"amount\":10,\"denom\":\"atom\"}]}])";
        EXPECT_TRUE(strcmp(buffer, cmp) == 0) << "Incorrect name, expected:" << cmp << ", received:" << buffer;
    }

    TEST(TransactionParserTest, read_msg_bytes_item_2_level_0) {
        auto transaction = R"({"alt_bytes":null,"chain_id":"test-chain-1","fee_bytes":{"amount":[{"amount":5,"denom":"photon"}],"gas":10000},"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]},"sequences":[1]})";
        char errorMsg[20];

        parsed_json_t parsed_json;
        json_parse(&parsed_json, transaction);

        // First find the msg_bytes parent token, level 0
        int token_index = json_get_child_token(
                transaction,
                &parsed_json,
                "msg_bytes",
                0);

        // Because token_index+1 is object
        int child_token_level_0 = json_get_child_token_by_index(
                transaction,
                &parsed_json,
                0,
                token_index+1);

        char buffer[100];
        json_read_token(transaction, &parsed_json, child_token_level_0, buffer, sizeof(buffer), 0);
        auto cmp = "outputs";
        EXPECT_TRUE(strcmp(buffer, cmp) == 0) << "Incorrect name, expected:" << cmp << ", received:" << buffer;

        // Because child_token_level_0+1 is object
        int child_token_level_1 = json_get_child_token_by_index(
                transaction,
                &parsed_json,
                0,
                child_token_level_0+1);

        json_read_token(transaction, &parsed_json, child_token_level_1, buffer, sizeof(buffer), 0);
        cmp = "address";
        EXPECT_TRUE(strcmp(buffer, cmp) == 0) << "Incorrect name, expected:" << cmp << ", received:" << buffer;

        // Because child_token_level_1+1 is string
        json_read_token(transaction, &parsed_json, child_token_level_1+1, buffer, sizeof(buffer), 0);
        cmp = "6F7574707574";
        EXPECT_TRUE(strcmp(buffer, cmp) == 0) << "Incorrect name, expected:" << cmp << ", received:" << buffer;
    }
}