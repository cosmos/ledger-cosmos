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
#include "common.h"
#include "common/parser.h"
#include <string>
#include <vector>

// value_substitutions turns an amino type into a readable action word. It is
// meant for msgs/N/type only: applied to any value, a field whose contents
// happen to equal a table entry is rewritten, so the screen shows a word that
// is not what was signed.
namespace {

std::string send_msg(const std::string &to, const std::string &memo) {
  return R"({"account_number":"8","chain_id":"cosmoshub-4",)"
         R"("fee":{"amount":[{"amount":"5000","denom":"uatom"}],"gas":"200000"},)"
         R"("memo":")" + memo + R"(","msgs":[{"type":"cosmos-sdk/MsgSend","value":{)"
         R"("amount":[{"amount":"10","denom":"uatom"}],)"
         R"("from_address":"cosmos1from","to_address":")" + to + R"("}}],"sequence":"1"})";
}

std::vector<std::string> render(const std::string &tx) {
  parser_context_t ctx;
  parser_tx_t tx_obj;
  memset(&tx_obj, 0, sizeof(tx_obj));
  tx_obj.tx_type = tx_json;

  auto err = parser_parse(&ctx, (const uint8_t *)tx.c_str(), tx.size(), &tx_obj);
  if (err != parser_ok) {
    return {};
  }
  if (parser_validate(&ctx) != parser_ok) {
    return {};
  }
  return dumpUI(&ctx, 40, 40);
}

// Joins every page of the screen with the given key.
std::string value_of(const std::vector<std::string> &ui, const std::string &key) {
  std::string joined;
  for (const auto &line : ui) {
    const auto bar = line.find(" | ");
    const auto colon = line.find(" : ");
    if (bar == std::string::npos || colon == std::string::npos || colon < bar) {
      continue;
    }
    std::string k = line.substr(bar + 3, colon - bar - 3);
    const auto page = k.find(" [");
    if (page != std::string::npos) {
      k = k.substr(0, page);
    }
    if (k == key) {
      joined += line.substr(colon + 3);
    }
  }
  return joined;
}

}  // namespace

// The behaviour the table exists for.
TEST(Substitution, MessageTypeIsStillRenderedAsAnActionWord) {
  app_mode_set_expert(false);
  const auto ui = render(send_msg("cosmos1to", "hello"));

  ASSERT_FALSE(ui.empty());
  EXPECT_EQ(value_of(ui, "Type"), "Send");
}

TEST(Substitution, RecipientMatchingATypeNameIsRenderedVerbatim) {
  app_mode_set_expert(false);
  const std::string type_lookalike = "/babylon.epoching.v1.MsgWrappedDelegate";
  const auto ui = render(send_msg(type_lookalike, "hello"));

  ASSERT_FALSE(ui.empty());
  EXPECT_EQ(value_of(ui, "To"), type_lookalike)
      << "recipient was rewritten as an action word";
  EXPECT_EQ(value_of(ui, "Type"), "Send");
}

TEST(Substitution, MemoMatchingATypeNameIsRenderedVerbatim) {
  app_mode_set_expert(false);
  const auto ui = render(send_msg("cosmos1to", "cosmos-sdk/MsgSend"));

  ASSERT_FALSE(ui.empty());
  EXPECT_EQ(value_of(ui, "Memo"), "cosmos-sdk/MsgSend");
}

TEST(Substitution, OrdinaryValuesAreUnaffected) {
  app_mode_set_expert(false);
  const auto ui = render(send_msg("cosmos1to", "hello"));

  ASSERT_FALSE(ui.empty());
  EXPECT_EQ(value_of(ui, "To"), "cosmos1to");
  EXPECT_EQ(value_of(ui, "Memo"), "hello");
  EXPECT_EQ(value_of(ui, "From"), "cosmos1from");
}
