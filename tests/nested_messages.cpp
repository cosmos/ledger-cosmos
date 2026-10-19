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
#include <vector>

// A MsgExec carries whole messages of its own, two object levels below where a
// plain message keeps its fields. The walk ran out of budget at the inner
// "value" and rendered the message the grantee is about to execute as one JSON
// blob -- so the screen said "a MsgExec is being authorised" and never said to
// whom, or how much. That is the shape a drainer wants: the recipient and the
// amount are in the document, and were never given a screen of their own.
namespace {

std::string exec_doc(const std::string &inner) {
  return R"({"account_number":"8","chain_id":"cosmoshub-4",)"
         R"("fee":{"amount":[{"amount":"5000","denom":"uatom"}],"gas":"200000"},)"
         R"("memo":"","msgs":[{"type":"cosmos-sdk/MsgExec","value":{)"
         R"("grantee":"cosmos1grantee","msgs":[)" + inner +
         R"(]}}],"sequence":"1"})";
}

const char *kInnerSend =
    R"({"type":"cosmos-sdk/MsgSend","value":{)"
    R"("amount":[{"amount":"1000000","denom":"uatom"}],)"
    R"("from_address":"cosmos1victim","to_address":"cosmos1attacker"}})";

// A grantee batch: one MsgExec carrying `count` distinct MsgSend messages.
// Distinct addresses per message, so neither the msg-type nor the msg-from
// grouping collapses screens and the item count grows with the batch.
std::string exec_batch(size_t count) {
  std::string inner;
  for (size_t i = 0; i < count; i++) {
    if (i > 0) {
      inner += ",";
    }
    const std::string n = std::to_string(100000 + i);
    inner += R"({"type":"cosmos-sdk/MsgSend","value":{)"
             R"("amount":[{"amount":"1000000","denom":"uatom"}],)"
             R"("from_address":"cosmos1victim)" + n + R"(",)"
             R"("to_address":"cosmos1grantee)" + n + R"("}})";
  }
  return exec_doc(inner);
}

struct Rendered {
  parser_error_t err;
  std::vector<std::string> keys;
  std::vector<std::string> values;
};

Rendered render_all(const std::string &tx) {
  parser_context_t ctx;
  parser_tx_t tx_obj;
  memset(&tx_obj, 0, sizeof(tx_obj));
  tx_obj.tx_type = tx_json;

  Rendered r{};
  r.err = parser_parse(&ctx, (const uint8_t *)tx.c_str(), tx.size(), &tx_obj);
  if (r.err != parser_ok) {
    return r;
  }
  r.err = parser_validate(&ctx);
  if (r.err != parser_ok) {
    return r;
  }

  uint8_t numItems = 0;
  r.err = parser_getNumItems(&ctx, &numItems);
  if (r.err != parser_ok) {
    return r;
  }

  for (uint8_t i = 0; i < numItems; i++) {
    char key[160] = {0};
    char val[320] = {0};
    uint8_t pageCount = 0;
    r.err = parser_getItem(&ctx, i, key, sizeof(key), val, sizeof(val), 0,
                           &pageCount);
    if (r.err != parser_ok) {
      return r;
    }
    r.keys.emplace_back(key);
    r.values.emplace_back(val);
  }
  return r;
}

bool has_key(const Rendered &r, const std::string &key) {
  for (const auto &k : r.keys) {
    if (k == key) {
      return true;
    }
  }
  return false;
}

std::string value_for(const Rendered &r, const std::string &key) {
  for (size_t i = 0; i < r.keys.size(); i++) {
    if (r.keys[i] == key) {
      return r.values[i];
    }
  }
  return "";
}

}  // namespace

TEST(NestedMessages, ExecInnerRecipientGetsItsOwnScreen) {
  const auto r = render_all(exec_doc(kInnerSend));
  ASSERT_EQ(r.err, parser_ok);
  ASSERT_TRUE(has_key(r, "msgs/value/msgs/value/to_address"))
      << "the recipient of the executed message has no screen";
  EXPECT_EQ(value_for(r, "msgs/value/msgs/value/to_address"), "cosmos1attacker");
}

TEST(NestedMessages, ExecInnerSenderGetsItsOwnScreen) {
  const auto r = render_all(exec_doc(kInnerSend));
  ASSERT_EQ(r.err, parser_ok);
  EXPECT_EQ(value_for(r, "msgs/value/msgs/value/from_address"), "cosmos1victim");
}

TEST(NestedMessages, ExecInnerTypeGetsItsOwnScreen) {
  const auto r = render_all(exec_doc(kInnerSend));
  ASSERT_EQ(r.err, parser_ok);
  EXPECT_EQ(value_for(r, "msgs/value/msgs/type"), "cosmos-sdk/MsgSend");
}

// Amount and denom stay on one screen deliberately: splitting them would put a
// bare "1000000" on a screen of its own, with the unit it is counted in on the
// next.
TEST(NestedMessages, ExecInnerAmountKeepsItsDenom) {
  const auto r = render_all(exec_doc(kInnerSend));
  ASSERT_EQ(r.err, parser_ok);
  const auto amount = value_for(r, "msgs/value/msgs/value/amount");
  EXPECT_NE(amount.find("1000000"), std::string::npos);
  EXPECT_NE(amount.find("uatom"), std::string::npos);
}

// The whole message must not come back as one undifferentiated page.
TEST(NestedMessages, ExecInnerMessageIsNotOneBlob) {
  const auto r = render_all(exec_doc(kInnerSend));
  ASSERT_EQ(r.err, parser_ok);
  EXPECT_FALSE(has_key(r, "msgs/value/msgs"))
      << "the executed message is still rendered as a single blob";
}

// The deeper budget belongs to MsgExec alone; an ordinary message keeps the
// depth it always had.
TEST(NestedMessages, OrdinarySendIsUnchanged) {
  const std::string tx =
      R"({"account_number":"8","chain_id":"cosmoshub-4",)"
      R"("fee":{"amount":[{"amount":"5000","denom":"uatom"}],"gas":"200000"},)"
      R"("memo":"","msgs":[{"type":"cosmos-sdk/MsgSend","value":{)"
      R"("amount":[{"amount":"10","denom":"uatom"}],)"
      R"("from_address":"cosmos1from","to_address":"cosmos1to"}}],"sequence":"1"})";
  const auto r = render_all(tx);
  ASSERT_EQ(r.err, parser_ok);
  EXPECT_TRUE(has_key(r, "To"));
  EXPECT_TRUE(has_key(r, "From"));
}

// A MsgExec with enough executed messages costs more review items than the
// review index can reach. Itemising them is what makes the batch legible, but a
// batch the device reviewed and signed before those screens existed must not
// become unsignable because of them: past the limit the executed messages go
// back to being one paged JSON value, which is what the app showed before.
//
// The boundary is exact, so it is pinned from both sides. In normal mode 31
// executed messages come to exactly MAX_REVIEW_ITEMS; expert mode adds four
// items of its own and moves the boundary down by one.
namespace {

constexpr size_t kLastItemisedNormal = 31;
constexpr size_t kLastItemisedExpert = 30;

}  // namespace

TEST(NestedMessages, ExecBatchAtTheIndexLimitKeepsItsScreens) {
  app_mode_set_expert(false);
  const auto r = render_all(exec_batch(kLastItemisedNormal));
  ASSERT_EQ(r.err, parser_ok);
  EXPECT_EQ(r.keys.size(), 127u) << "the boundary moved; re-derive it";
  EXPECT_TRUE(has_key(r, "msgs/value/msgs/value/to_address"))
      << "the last batch that fits lost its itemised screens";
  EXPECT_FALSE(has_key(r, "msgs/value/msgs"))
      << "the executed messages fell back to a blob before the limit";
}

TEST(NestedMessages, ExecBatchPastTheIndexLimitIsStillSignable) {
  app_mode_set_expert(false);
  const auto r = render_all(exec_batch(kLastItemisedNormal + 1));
  ASSERT_EQ(r.err, parser_ok)
      << "a batch that reviewed and signed before the executed messages were "
         "itemised is now refused: " << parser_getErrorDescription(r.err);
  EXPECT_TRUE(has_key(r, "msgs/value/msgs"))
      << "past the limit the executed messages must render as one paged value";
  EXPECT_FALSE(has_key(r, "msgs/value/msgs/value/to_address"))
      << "itemised screens survived past the limit; the count and the "
         "rendering disagree";
}

TEST(NestedMessages, ExecBatchBoundaryInExpertMode) {
  app_mode_set_expert(true);
  const auto fits = render_all(exec_batch(kLastItemisedExpert));
  const auto over = render_all(exec_batch(kLastItemisedExpert + 1));
  app_mode_set_expert(false);

  ASSERT_EQ(fits.err, parser_ok);
  EXPECT_EQ(fits.keys.size(), 127u) << "the expert boundary moved";
  EXPECT_TRUE(has_key(fits, "msgs/value/msgs/value/to_address"));

  ASSERT_EQ(over.err, parser_ok)
      << "expert mode refuses a batch normal mode signs";
  EXPECT_TRUE(has_key(over, "msgs/value/msgs"));
}

// The fallback is a property of the document, taken before any item is
// rendered. If counting and rendering could disagree about it, the review would
// ask for an item the walk no longer produces.
TEST(NestedMessages, EveryCountedItemRendersOnBothSidesOfTheBoundary) {
  app_mode_set_expert(false);
  for (size_t count : {kLastItemisedNormal, kLastItemisedNormal + 1}) {
    const auto r = render_all(exec_batch(count));
    ASSERT_EQ(r.err, parser_ok) << count << " executed messages";
    EXPECT_EQ(r.keys.size(), r.values.size());
    for (const auto &key : r.keys) {
      EXPECT_FALSE(key.empty()) << "an item rendered without a key at " << count;
    }
  }
}

// Falling back is not truncating: the executed messages are still shown in
// full, one page at a time, exactly as they were before they had screens.
TEST(NestedMessages, FallbackBlobCarriesEveryExecutedMessage) {
  app_mode_set_expert(false);
  const size_t count = kLastItemisedNormal + 1;
  const std::string tx = exec_batch(count);

  parser_context_t ctx;
  parser_tx_t tx_obj;
  memset(&tx_obj, 0, sizeof(tx_obj));
  tx_obj.tx_type = tx_json;
  ASSERT_EQ(parser_parse(&ctx, (const uint8_t *)tx.c_str(), tx.size(), &tx_obj),
            parser_ok);
  ASSERT_EQ(parser_validate(&ctx), parser_ok);

  uint8_t numItems = 0;
  ASSERT_EQ(parser_getNumItems(&ctx, &numItems), parser_ok);

  std::string blob;
  for (uint8_t i = 0; i < numItems; i++) {
    char key[160] = {0};
    char val[160] = {0};
    uint8_t pageCount = 0;
    ASSERT_EQ(parser_getItem(&ctx, i, key, sizeof(key), val, sizeof(val), 0,
                             &pageCount),
              parser_ok);
    if (std::string(key) != "msgs/value/msgs") {
      continue;
    }
    for (uint8_t page = 0; page < pageCount; page++) {
      char pageKey[160] = {0};
      char pageVal[160] = {0};
      uint8_t pages = 0;
      ASSERT_EQ(parser_getItem(&ctx, i, pageKey, sizeof(pageKey), pageVal,
                               sizeof(pageVal), page, &pages),
                parser_ok);
      blob += pageVal;
    }
  }

  ASSERT_FALSE(blob.empty()) << "the executed messages were never rendered";
  const std::string last =
      "cosmos1grantee" + std::to_string(100000 + count - 1);
  EXPECT_NE(blob.find(last), std::string::npos)
      << "the last executed message never reached a page";
}

// A batch too large to review even as a blob stays refused: the fallback only
// ever chooses between two renderings, never between reviewing and truncating.
TEST(NestedMessages, ExecFallbackDoesNotRescueAnOversizedDocument) {
  app_mode_set_expert(false);
  std::string msgs;
  for (size_t i = 0; i < 70; i++) {
    if (i > 0) {
      msgs += ",";
    }
    const std::string n = std::to_string(100000 + i);
    msgs += R"({"type":"cosmos-sdk/MsgWithdrawDelegationReward","value":{)"
            R"("delegator_address":"cosmos1delegator)" + n + R"(",)"
            R"("validator_address":"cosmosvaloper1validator)" + n + R"("}})";
  }
  const std::string tx =
      R"({"account_number":"8","chain_id":"cosmoshub-4",)"
      R"("fee":{"amount":[{"amount":"5000","denom":"uatom"}],"gas":"200000"},)"
      R"("memo":"","msgs":[)" + msgs + R"(],"sequence":"1"})";

  const auto r = render_all(tx);
  EXPECT_NE(r.err, parser_ok);
}
