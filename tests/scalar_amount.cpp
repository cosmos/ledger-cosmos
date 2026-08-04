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

// Amounts are recognised by their flattened path, not their type, so
// msgs/value/amount reaches the Coin formatter whatever shape it holds. Several
// released message types outside cosmos-sdk core encode that field as
// cosmossdk.io/math.Int, which amino writes as a bare quoted integer:
// cosmos/evm and canto MsgConvertERC20, Kava's MsgConvertERC20ToCoin, Stride's
// stakeibc messages, Injective's peggy claims among them.
//
// A scalar leaf reports zero children exactly as an empty container does, so
// the Coin formatter used to answer "Empty" and the device displayed a benign
// blank for a value it went on to sign.
namespace {

std::string with_amount(const std::string &amount_json) {
  return R"({"account_number":"8","chain_id":"cosmoshub-4",)"
         R"("fee":{"amount":[{"amount":"5000","denom":"uatom"}],"gas":"200000"},)"
         R"("memo":"","msgs":[{"type":"cosmos/evm/MsgConvertERC20","value":{)"
         R"("amount":)" +
         amount_json +
         R"(,"contract_address":"0x0000000000000000000000000000000000000042",)"
         R"("receiver":"cosmos1receiver","sender":"cosmos1sender"}}],)"
         R"("sequence":"1"})";
}

// Render every item and return what the Amount row shows.
std::string amount_shown(const parser_context_t *ctx) {
  uint8_t num_items = 0;
  if (parser_getNumItems(ctx, &num_items) != parser_ok) {
    return "<getNumItems failed>";
  }

  char key[64];
  char val[300];
  for (uint8_t idx = 0; idx < num_items; idx++) {
    uint8_t page_count = 0;
    if (parser_getItem(ctx, idx, key, sizeof(key), val, sizeof(val), 0,
                       &page_count) != parser_ok) {
      return "<getItem failed>";
    }
    if (std::string(key) == "Amount") {
      return std::string(val);
    }
  }
  return "<no Amount row>";
}

parser_error_t run(const std::string &tx, parser_context_t *ctx,
                   parser_tx_t *tx_obj) {
  memset(tx_obj, 0, sizeof(*tx_obj));
  tx_obj->tx_type = tx_json;

  auto err = parser_parse(ctx, (const uint8_t *)tx.c_str(), tx.size(), tx_obj);
  if (err != parser_ok) {
    return err;
  }
  return parser_validate(ctx);
}

}  // namespace

// The defect: a scalar amount reached the user as "Empty" while the real value
// was signed. It must now be shown.
TEST(ScalarAmount, ScalarAmountIsShownNotReportedEmpty) {
  app_mode_set_expert(false);

  const struct {
    const char *json;
    const char *shown;
  } cases[] = {
      {R"("10")", "10"},
      {R"("100")", "100"},
      {R"("1")", "1"},
      {R"("1000000000000000000")", "1000000000000000000"},
      {R"(10)", "10"},
  };

  for (const auto &c : cases) {
    const std::string tx = with_amount(c.json);

    parser_context_t ctx;
    parser_tx_t tx_obj;
    ASSERT_EQ(run(tx, &ctx, &tx_obj), parser_ok)
        << "a scalar amount is a normal encoding and must stay signable: "
        << c.json;

    EXPECT_EQ(amount_shown(&ctx), std::string(c.shown))
        << "scalar amount " << c.json << " was not shown faithfully";
  }
}

// Two different hidden amounts used to produce an identical review. The screen
// has to tell them apart, or the signature is not what the user consented to.
TEST(ScalarAmount, DifferentScalarAmountsLookDifferent) {
  app_mode_set_expert(false);

  const std::string tx_a = with_amount(R"("10")");
  const std::string tx_b = with_amount(R"("100")");

  parser_context_t ctx_a;
  parser_tx_t obj_a;
  ASSERT_EQ(run(tx_a, &ctx_a, &obj_a), parser_ok);
  const std::string shown_a = amount_shown(&ctx_a);

  parser_context_t ctx_b;
  parser_tx_t obj_b;
  ASSERT_EQ(run(tx_b, &ctx_b, &obj_b), parser_ok);
  const std::string shown_b = amount_shown(&ctx_b);

  EXPECT_NE(shown_a, shown_b)
      << "both amounts rendered as \"" << shown_a
      << "\", so the review cannot show which one is being signed";
  EXPECT_NE(shown_a, "Empty");
  EXPECT_NE(shown_b, "Empty");
}

// The shapes a Coin really takes must keep working.
TEST(ScalarAmount, GenuineCoinShapesStillRender) {
  app_mode_set_expert(false);

  // The parser keeps a pointer into the document, so each one has to outlive
  // every call that reads it back. Bind it, never pass a temporary.

  // A one-element Coin array is the ordinary case.
  {
    const std::string tx = with_amount(R"([{"amount":"10","denom":"atest"}])");
    parser_context_t ctx;
    parser_tx_t tx_obj;
    ASSERT_EQ(run(tx, &ctx, &tx_obj), parser_ok);
    EXPECT_EQ(amount_shown(&ctx), "10 atest");
  }

  // An empty Coin array is a real empty collection and still reads as Empty.
  {
    const std::string tx = with_amount("[]");
    parser_context_t ctx;
    parser_tx_t tx_obj;
    ASSERT_EQ(run(tx, &ctx, &tx_obj), parser_ok);
    EXPECT_EQ(amount_shown(&ctx), "Empty");
  }

  // A single Coin object, not wrapped in an array.
  {
    const std::string tx = with_amount(R"({"amount":"7","denom":"atest"})");
    parser_context_t ctx;
    parser_tx_t tx_obj;
    ASSERT_EQ(run(tx, &ctx, &tx_obj), parser_ok);
    EXPECT_EQ(amount_shown(&ctx), "7 atest");
  }
}

// A non-object inside a Coin array is a malformed list, not an unmodelled
// shape, and is still refused.
TEST(ScalarAmount, MalformedCoinArrayIsStillRefused) {
  app_mode_set_expert(false);

  const std::string tx = with_amount(R"(["10"])");

  parser_context_t ctx;
  parser_tx_t tx_obj;
  EXPECT_NE(run(tx, &ctx, &tx_obj), parser_ok)
      << "a bare string inside an amount array should not be accepted";
}
