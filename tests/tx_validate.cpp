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
#include <json/json_parser.h>
#include <tx_validate.h>
#include <common/parser.h>
#include "common.h"

namespace {
    TEST(TxValidationTest, CorrectFormat) {
        auto transaction =
            R"({"account_number":"0","chain_id":"test-chain-1","fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";

        parsed_json_t json;
        parser_error_t err;

        err = JSON_PARSE(&json, transaction);
        ASSERT_EQ(err, parser_ok);

        err = tx_validate(&json);
        EXPECT_EQ(err, parser_ok) << "Validation failed, error: " << parser_getErrorDescription(err);
    }

    TEST(TxValidationTest, MissingAccountNumber) {
        auto transaction =
            R"({"chain_id":"test-chain-1","fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";

        parsed_json_t json;
        parser_error_t err;

        err = JSON_PARSE(&json, transaction);
        ASSERT_EQ(err, parser_ok);

        err = tx_validate(&json);

        EXPECT_EQ(err, parser_json_missing_account_number) << "Validation failed, error: " << parser_getErrorDescription(err);
    }

    TEST(TxValidationTest, MissingChainId) {
        auto transaction =
            R"({"account_number":"0","fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";

        parsed_json_t json;
        parser_error_t err;

        err = JSON_PARSE(&json, transaction);
        ASSERT_EQ(err, parser_ok);

        err = tx_validate(&json);
        EXPECT_EQ(err, parser_json_missing_chain_id) << "Validation failed, error: " << parser_getErrorDescription(err);
    }

    TEST(TxValidationTest, MissingFee) {
        auto transaction =
            R"({"account_number":"0","chain_id":"test-chain-1","fees":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";

        parsed_json_t json;
        parser_error_t err;

        err = JSON_PARSE(&json, transaction);
        ASSERT_EQ(err, parser_ok);

        err = tx_validate(&json);
        EXPECT_EQ(err, parser_json_missing_fee) << "Validation failed, error: " << parser_getErrorDescription(err);
    }

    TEST(TxValidationTest, MissingMsgs) {
        auto transaction =
            R"({"account_number":"0","chain_id":"test-chain-1","fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgsble":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";

        parsed_json_t json;
        parser_error_t err;

        err = JSON_PARSE(&json, transaction);
        ASSERT_EQ(err, parser_ok);

        err = tx_validate(&json);
        EXPECT_EQ(err, parser_json_missing_msgs) << "Validation failed, error: " << parser_getErrorDescription(err);
    }

    TEST(TxValidationTest, MissingSequence) {
        auto transaction =
            R"({"account_number":"0","chain_id":"test-chain-1","fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}]})";

        parsed_json_t json;
        parser_error_t err;

        err = JSON_PARSE(&json, transaction);
        ASSERT_EQ(err, parser_ok);

        err = tx_validate(&json);
        EXPECT_EQ(err, parser_json_missing_sequence) << "Validation failed, error: " << parser_getErrorDescription(err);
    }

    TEST(TxValidationTest, Spaces_InTheMiddle) {
        auto transaction =
            R"({"account_number":"0","chain_id":"test-chain-1", "fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";

        parsed_json_t json;
        parser_error_t err;

        err = JSON_PARSE(&json, transaction);
        ASSERT_EQ(err, parser_ok);

        err = tx_validate(&json);
        EXPECT_EQ(err, parser_json_contains_whitespace) << "Validation failed, error: " << parser_getErrorDescription(err);
    }

    TEST(TxValidationTest, Spaces_AtTheFront) {
        auto transaction =
            R"({  "account_number":"0","chain_id":"test-chain-1","fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";

        parsed_json_t json;
        parser_error_t err;

        err = JSON_PARSE(&json, transaction);
        ASSERT_EQ(err, parser_ok);

        err = tx_validate(&json);
        EXPECT_EQ(err, parser_json_contains_whitespace) << "Validation failed, error: " << parser_getErrorDescription(err);
    }

    TEST(TxValidationTest, Spaces_AtTheEnd) {
        auto transaction =
            R"({"account_number":"0","chain_id":"test-chain-1","fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"  })";

        parsed_json_t json;
        parser_error_t err;

        err = JSON_PARSE(&json, transaction);
        ASSERT_EQ(err, parser_ok);

        err = tx_validate(&json);
        EXPECT_EQ(err, parser_json_contains_whitespace) << "Validation failed, error: " << parser_getErrorDescription(err);
    }

    TEST(TxValidationTest, Spaces_Lots) {
        auto transaction =
            R"({"account_number":"0",   "chain_id":"test-chain-1",    "fee":{"amount":    [{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";

        parsed_json_t json;
        parser_error_t err;

        err = JSON_PARSE(&json, transaction);
        ASSERT_EQ(err, parser_ok);

        err = tx_validate(&json);
        EXPECT_EQ(err, parser_json_contains_whitespace) << "Validation failed, error: " << parser_getErrorDescription(err);
    }

    TEST(TxValidationTest, AllowSpacesInString) {
        auto transaction =
            R"({"account_number":"0","chain_id":"    test-chain-1    ","fee":{"amount":[{"amount":"5","denom":"    photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";

        parsed_json_t json;
        parser_error_t err;

        err = JSON_PARSE(&json, transaction);
        ASSERT_EQ(err, parser_ok);

        err = tx_validate(&json);
        EXPECT_EQ(err, parser_ok) << "Validation failed, error: " << parser_getErrorDescription(err);
    }

    TEST(TxValidationTest, SortedDictionary) {
        auto transaction =
            R"({"account_number":"0","chain_id":"test-chain-1","fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";

        parsed_json_t json;
        parser_error_t err;

        err = JSON_PARSE(&json, transaction);
        ASSERT_EQ(err, parser_ok);

        err = tx_validate(&json);
        EXPECT_EQ(err, parser_ok) << "Validation failed, error: " << parser_getErrorDescription(err);
    }

    TEST(TxValidationTest, NotSortedDictionary_FirstElement) {
        auto transaction =
            R"({"chain_id":"test-chain-1","account_number":"0","fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";

        parsed_json_t json;
        parser_error_t err;

        err = JSON_PARSE(&json, transaction);
        ASSERT_EQ(err, parser_ok);

        err = tx_validate(&json);
        EXPECT_EQ(err, parser_json_is_not_sorted) << "Validation failed, error: " << parser_getErrorDescription(err);
    }

    TEST(TxValidationTest, NotSortedDictionary_MiddleElement) {
        auto transaction =
            R"({"account_number":"0","chain_id":"test-chain-1","memo":"testmemo","fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";

        parsed_json_t json;
        parser_error_t err;

        err = JSON_PARSE(&json, transaction);
        ASSERT_EQ(err, parser_ok);

        err = tx_validate(&json);
        EXPECT_EQ(err, parser_json_is_not_sorted) << "Validation failed, error: " << parser_getErrorDescription(err);
    }

    TEST(TxValidationTest, NotSortedDictionary_LastElement) {
        auto transaction =
            R"({"account_number":"0","chain_id":"test-chain-1","fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","sequence":"1","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}]})";

        parsed_json_t json;
        parser_error_t err;

        err = JSON_PARSE(&json, transaction);
        ASSERT_EQ(err, parser_ok);

        err = tx_validate(&json);
        EXPECT_EQ(err, parser_json_is_not_sorted) << "Validation failed, error: " << parser_getErrorDescription(err);
    }

// This json has been taken directly from goclient which uses cosmos to serialize a simple tx
// This test is currently failing the validation.
// We are reviewing the validation code and cosmos serialization to find the culprit.

    TEST(TxValidationTest, CosmosExample) {
        auto transaction =
            R"({"account_number":"0","chain_id":"test-chain-1","fee":{"amount":[{"amount":"5","denom":"photon"}],"gas":"10000"},"memo":"testmemo","msgs":[{"inputs":[{"address":"cosmosaccaddr1d9h8qat5e4ehc5","coins":[{"amount":"10","denom":"atom"}]}],"outputs":[{"address":"cosmosaccaddr1da6hgur4wse3jx32","coins":[{"amount":"10","denom":"atom"}]}]}],"sequence":"1"})";

        parsed_json_t json;
        parser_error_t err;

        err = JSON_PARSE(&json, transaction);
        ASSERT_EQ(err, parser_ok);

        err = tx_validate(&json);
        EXPECT_EQ(err, parser_ok) << "Validation failed, error: " << parser_getErrorDescription(err);
    }

    TEST(TxValidationTest, GaiaCLIissue) {
        auto transaction = R"({"account_number":"811","chain_id":"cosmoshub-1","fee":{"amount":[],"gas":"5000000"},"memo":"","msgs":[{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper14kn0kk33szpwus9nh8n87fjel8djx0y070ymmj","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper14kn0kk33szpwus9nh8n87fjel8djx0y070ymmj","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper14kn0kk33szpwus9nh8n87fjel8djx0y070ymmj","value":{"amount":"8000000000","denom":"uatom"}}}],"sequence":"1"})";

        parsed_json_t json;
        parser_error_t err;

        err = JSON_PARSE(&json, transaction);
        ASSERT_EQ(err, parser_ok);

        err = tx_validate(&json);
        EXPECT_EQ(err, parser_ok) << "Validation failed, error: " << parser_getErrorDescription(err);
    }

    TEST(TxValidationTest, GaiaCLIissueBigTX) {
        auto transaction = R"({"account_number":"811","chain_id":"cosmoshub-1","fee":{"amount":[],"gas":"5000000"},"memo":"","msgs":[{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx",
  "validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx",
  "validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx",
  "validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx",
"validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx",
  "validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx",
  "validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx",
"validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx",
  "validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx",
  "validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx",
  "validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx",
  "validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx",
  "validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx",
  "validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx",
  "validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx",
  "validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx",
  "validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx",
  "validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx",
  "validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx",
  "validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx",
  "validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx",
  "validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx",
  "validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper10e4vsut6suau8tk9m6dnrm0slgd6npe3jx5xpv","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper14kn0kk33szpwus9nh8n87fjel8djx0y070ymmj","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper14kn0kk33szpwus9nh8n87fjel8djx0y070ymmj","value":{"amount":"8000000000","denom":"uatom"}}},{"type":"cosmos-sdk/MsgDelegate","value":{"delegator_address":"cosmos13vfzpfmg6jgzfk4rke9glzpngrzucjtanq9awx","validator_address":"cosmosvaloper14kn0kk33szpwus9nh8n87fjel8djx0y070ymmj","value":{"amount":"8000000000","denom":"uatom"}}}],"sequence":"1"})";

        parsed_json_t json;
        parser_error_t err;

        err = JSON_PARSE(&json, transaction);
        ASSERT_EQ( err, parser_json_too_many_tokens);

        err = tx_validate(&json);
        EXPECT_EQ(err, parser_json_missing_chain_id) << "Validation failed, error: " << parser_getErrorDescription(err);
    }
}
