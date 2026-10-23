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

// An object or array the walk cannot split is shown as one value, the JSON
// text itself. Its escapes must stay: decoded, a quote inside one of its
// strings is indistinguishable from a quote that is structure, and a string
// field placed before an address can make the screen read as if the address
// were somewhere else.
namespace {

struct Item {
  std::string key;
  std::string value;  // all pages joined
};

struct Rendered {
  parser_error_t err;
  std::vector<Item> items;
};

Rendered render_all(const std::string &tx, bool expert, uint16_t pageLen) {
  app_mode_set_expert(expert);
  parser_context_t ctx;
  parser_tx_t tx_obj;
  memset(&tx_obj, 0, sizeof(tx_obj));
  tx_obj.tx_type = tx_json;

  Rendered r{};
  r.err = parser_parse(&ctx, (const uint8_t *)tx.c_str(), tx.size(), &tx_obj);
  if (r.err == parser_ok) r.err = parser_validate(&ctx);
  uint8_t numItems = 0;
  if (r.err == parser_ok) r.err = parser_getNumItems(&ctx, &numItems);

  for (uint8_t i = 0; r.err == parser_ok && i < numItems; i++) {
    Item it;
    uint8_t pageCount = 1;
    for (uint8_t p = 0; r.err == parser_ok && p < pageCount; p++) {
      char key[64] = {0};
      std::vector<char> val(pageLen, 0);
      r.err = parser_getItem(&ctx, i, key, sizeof(key), val.data(), pageLen, p,
                             &pageCount);
      it.key = key;
      it.value += val.data();
    }
    r.items.push_back(it);
  }
  app_mode_set_expert(false);
  return r;
}

const Item *find(const Rendered &r, const std::string &key) {
  for (const auto &it : r.items) {
    if (it.key == key) return &it;
  }
  return nullptr;
}

// wasm MsgExecuteContract: `msg` is an inline JSON object, shown whole at
// msgs/value/msg. "memo" sorts before "remote_address".
std::string wasm_doc(const std::string &inline_msg) {
  return R"({"account_number":"8","chain_id":"juno-1",)"
         R"("fee":{"amount":[{"amount":"5000","denom":"ujuno"}],"gas":"200000"},)"
         R"("memo":"","msgs":[{"type":"wasm/MsgExecuteContract","value":{)"
         R"("contract":"juno1ics20contract",)"
         R"("funds":[{"amount":"250000000","denom":"ujuno"}],)"
         R"("msg":)" + inline_msg + R"(,"sender":"juno1victim"}}],"sequence":"1"})";
}

// Decoded, this memo closes the transfer object after a fake remote_address.
const std::string kForgingMemo =
    R"(\",\"remote_address\":\"cosmos1friend\"},\"x\":{\"y\":\")";

std::string forging_msg() {
  return R"({"transfer":{"channel":"channel-1","memo":")" + kForgingMemo +
         R"(","remote_address":"cosmos1attacker","timeout":3600}})";
}

}  // namespace

TEST(JsonBlobRaw, PlainObjectIsShownVerbatim) {
  const std::string msg =
      R"({"transfer":{"channel":"channel-1","memo":"hi",)"
      R"("remote_address":"cosmos1friend","timeout":3600}})";
  const auto r = render_all(wasm_doc(msg), false, 64);
  ASSERT_EQ(r.err, parser_ok);
  const Item *it = find(r, "msgs/value/msg");
  ASSERT_NE(it, nullptr);
  EXPECT_EQ(it->value, msg);
}

TEST(JsonBlobRaw, EscapedQuotesInsideAnObjectStayEscaped) {
  for (const bool expert : {false, true}) {
    const auto r = render_all(wasm_doc(forging_msg()), expert, 64);
    ASSERT_EQ(r.err, parser_ok);
    const Item *it = find(r, "msgs/value/msg");
    ASSERT_NE(it, nullptr);
    EXPECT_EQ(it->value, forging_msg()) << "expert=" << expert;
    EXPECT_EQ(it->value.find(R"("remote_address":"cosmos1friend"},)"),
              std::string::npos);
  }
}

TEST(JsonBlobRaw, EscapedBackslashInsideAnObjectStaysEscaped) {
  const std::string msg =
      R"({"transfer":{"channel":"channel-1","memo":"a\\",)"
      R"("remote_address":"cosmos1attacker","timeout":3600}})";
  const auto r = render_all(wasm_doc(msg), false, 64);
  ASSERT_EQ(r.err, parser_ok);
  const Item *it = find(r, "msgs/value/msg");
  ASSERT_NE(it, nullptr);
  EXPECT_EQ(it->value, msg);
}

// A grantee batch too large to itemise falls back to showing the inner
// messages as one array. An IBC MsgTransfer inside it keeps memo < receiver.
TEST(JsonBlobRaw, MsgExecFallbackKeepsTheIbcReceiverWhereItIs) {
  std::string inner;
  for (int i = 0; i < 30; i++) {
    inner += R"({"type":"cosmos-sdk/MsgSend","value":{)"
             R"("amount":[{"amount":"1","denom":"uatom"}],)"
             R"("from_address":"cosmos1granter","to_address":"cosmos1payee)" +
             std::to_string(100000 + i) + R"("}},)";
  }
  const std::string memo =
      R"(\",\"receiver\":\"osmo1friend\"},\"forward\":{\"note\":\")";
  inner += R"({"type":"cosmos-sdk/MsgTransfer","value":{"memo":")" + memo +
           R"(","receiver":"osmo1attacker","sender":"cosmos1granter",)"
           R"("source_channel":"channel-141","source_port":"transfer",)"
           R"("timeout_height":{},"timeout_timestamp":"1700000000000000000",)"
           R"("token":{"amount":"5000000000","denom":"uatom"}}})";
  const std::string tx =
      R"({"account_number":"8","chain_id":"cosmoshub-4",)"
      R"("fee":{"amount":[{"amount":"5000","denom":"uatom"}],"gas":"200000"},)"
      R"("memo":"","msgs":[{"type":"cosmos-sdk/MsgExec","value":{)"
      R"("grantee":"cosmos1granter","msgs":[)" + inner + R"(]}}],"sequence":"1"})";

  const auto r = render_all(tx, false, 160);
  ASSERT_EQ(r.err, parser_ok);
  const Item *it = find(r, "msgs/value/msgs");
  ASSERT_NE(it, nullptr) << "the batch is expected to fall back to one value";
  EXPECT_NE(it->value.find(R"("memo":")" + memo + R"(","receiver":"osmo1attacker")"),
            std::string::npos);
  EXPECT_EQ(it->value.find(R"("receiver":"osmo1friend"},)"), std::string::npos);
}
