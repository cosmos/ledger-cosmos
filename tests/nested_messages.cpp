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
