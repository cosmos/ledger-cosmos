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

#include "app_mode.h"
#include "coin.h"
#include "common/parser.h"
#include <string>

// A Coin is an sdk.Int amount and a denom matching
// [a-zA-Z][a-zA-Z0-9/:._-]{2,127}. The display path used to check only that
// both were shorter than their buffers, so anything that fit was rendered as
// though it were spendable: "1.5", "-1", "0x10" and a padded look-alike of the
// unit the app shifts by six decimals all reached the screen looking like
// amounts. Rendering a value the chain will not accept invents a reading the
// signer can act on, so the coin path refuses it instead.
namespace {

std::string signdoc(const std::string &amount, const std::string &denom) {
  return R"({"account_number":"8","chain_id":"cosmoshub-4",)"
         R"("fee":{"amount":[{"amount":"5000","denom":"uatom"}],"gas":"200000"},)"
         R"("memo":"","msgs":[{"type":"cosmos-sdk/MsgSend","value":{)"
         R"("amount":[{"amount":")" + amount + R"(","denom":")" + denom + R"("}],)"
         R"("from_address":"cosmos1from","to_address":"cosmos1to"}}],"sequence":"1"})";
}

// The coin is only inspected where it is formatted for display, so render every
// item and report the first error one of them returns.
parser_error_t render(const std::string &tx) {
  parser_context_t ctx;
  parser_tx_t tx_obj;
  memset(&tx_obj, 0, sizeof(tx_obj));
  tx_obj.tx_type = tx_json;

  auto err = parser_parse(&ctx, (const uint8_t *)tx.c_str(), tx.size(), &tx_obj);
  if (err != parser_ok) {
    return err;
  }
  err = parser_validate(&ctx);
  if (err != parser_ok) {
    return err;
  }

  uint8_t numItems = 0;
  err = parser_getNumItems(&ctx, &numItems);
  if (err != parser_ok) {
    return err;
  }

  for (uint8_t i = 0; i < numItems; i++) {
    char key[128] = {0};
    char val[128] = {0};
    uint8_t pageCount = 0;
    err = parser_getItem(&ctx, i, key, sizeof(key), val, sizeof(val), 0,
                         &pageCount);
    if (err != parser_ok) {
      return err;
    }
  }
  return parser_ok;
}

}  // namespace

TEST(CoinValues, OrdinaryCoinIsAccepted) {
  EXPECT_EQ(render(signdoc("10", "uatom")), parser_ok);
}

TEST(CoinValues, ZeroIsAccepted) {
  EXPECT_EQ(render(signdoc("0", "uatom")), parser_ok);
}

TEST(CoinValues, IbcDenomIsAccepted) {
  EXPECT_EQ(render(signdoc("1", "ibc/27394FB092D2ECCD56123C74F36E4C1F"
                                "926001CEADA9CA97EA622B25F41E5EB2")),
            parser_ok);
}

TEST(CoinValues, FactoryDenomIsAccepted) {
  EXPECT_EQ(render(signdoc("1", "factory/cosmos1abc/uusd")), parser_ok);
}

// --- amounts the chain would not parse as an sdk.Int ---

TEST(CoinValues, FractionalAmountIsRejected) {
  EXPECT_EQ(render(signdoc("1.5", "uatom")), parser_unexpected_characters);
}

TEST(CoinValues, NegativeAmountIsRejected) {
  EXPECT_EQ(render(signdoc("-1", "uatom")), parser_unexpected_characters);
}

TEST(CoinValues, HexAmountIsRejected) {
  EXPECT_EQ(render(signdoc("0x10", "uatom")), parser_unexpected_characters);
}

TEST(CoinValues, ExponentAmountIsRejected) {
  EXPECT_EQ(render(signdoc("1e9", "uatom")), parser_unexpected_characters);
}

TEST(CoinValues, LeadingZeroIsRejected) {
  // "01" and "1" are the same number but not the same bytes; only one of them
  // is what a canonical document carries.
  EXPECT_EQ(render(signdoc("01", "uatom")), parser_unexpected_value);
}

TEST(CoinValues, AlphabeticAmountIsRejected) {
  EXPECT_EQ(render(signdoc("b", "uatom")), parser_unexpected_characters);
}

// --- denoms outside the SDK's charset ---

TEST(CoinValues, DenomWithTrailingSpaceIsRejected) {
  // The near-miss that matters: "uatom " is not "uatom", so it never takes the
  // six-decimal shift, and it used to render beside an unshifted amount.
  EXPECT_EQ(render(signdoc("1000000", "uatom ")), parser_unexpected_characters);
}

TEST(CoinValues, DenomStartingWithADigitIsRejected) {
  EXPECT_EQ(render(signdoc("1", "1atom")), parser_unexpected_characters);
}

TEST(CoinValues, TooShortDenomIsRejected) {
  EXPECT_EQ(render(signdoc("1", "d")), parser_unexpected_value);
}

TEST(CoinValues, DenomWithPunctuationIsRejected) {
  EXPECT_EQ(render(signdoc("1", "uatom!")), parser_unexpected_characters);
}

// Uppercase is inside the SDK charset, so it stays accepted. It is also what
// makes a value wide on a touch screen, which is a display concern handled in
// the review layer rather than here.
TEST(CoinValues, UppercaseDenomIsAccepted) {
  EXPECT_EQ(render(signdoc("1", "WBTC")), parser_ok);
}
