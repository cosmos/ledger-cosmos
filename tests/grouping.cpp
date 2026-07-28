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
#include <algorithm>
#include <string>
#include <vector>

// A batch of identical messages collapses its repeated Type and From screens
// into one. That is only sound while "identical" means the same signed bytes:
// several distinct amino types render through value_substitutions to the same
// friendly label, so comparing what is displayed would collapse messages the
// chain treats differently.
namespace {

std::string staking_msg(const std::string &type, const std::string &validator) {
  return R"({"type":")" + type + R"(","value":{)"
         R"("amount":{"amount":"1000000","denom":"uatom"},)"
         R"("delegator_address":"cosmos1delegator",)"
         R"("validator_address":")" + validator + R"("}})";
}

std::string signdoc(const std::vector<std::string> &msgs) {
  std::string joined;
  for (size_t i = 0; i < msgs.size(); i++) {
    if (i > 0) {
      joined += ",";
    }
    joined += msgs[i];
  }
  return R"({"account_number":"8","chain_id":"cosmoshub-4",)"
         R"("fee":{"amount":[{"amount":"5000","denom":"uatom"}],"gas":"200000"},)"
         R"("memo":"","msgs":[)" + joined + R"(],"sequence":"1"})";
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

// dumpUI emits "<idx> | <key>[ [p/n]] : <value>", one line per page. Count
// distinct item indices so a paginated value is still one screen.
size_t count_screens(const std::vector<std::string> &ui, const std::string &key) {
  std::vector<std::string> seen;

  for (const auto &line : ui) {
    const auto bar = line.find(" | ");
    const auto colon = line.find(" : ");
    if (bar == std::string::npos || colon == std::string::npos || colon < bar) {
      continue;
    }

    const std::string idx = line.substr(0, bar);
    std::string k = line.substr(bar + 3, colon - bar - 3);

    const auto page = k.find(" [");
    if (page != std::string::npos) {
      k = k.substr(0, page);
    }
    if (k != key) {
      continue;
    }
    if (std::find(seen.begin(), seen.end(), idx) == seen.end()) {
      seen.push_back(idx);
    }
  }
  return seen.size();
}

}  // namespace

// The behaviour grouping exists for, and which must not regress.
TEST(Grouping, IdenticalTypesStillCollapseToOneScreen) {
  app_mode_set_expert(false);

  const auto ui = render(signdoc({
      staking_msg("cosmos-sdk/MsgDelegate", "cosmosvaloper1aaa"),
      staking_msg("cosmos-sdk/MsgDelegate", "cosmosvaloper1bbb"),
      staking_msg("cosmos-sdk/MsgDelegate", "cosmosvaloper1ccc"),
  }));

  ASSERT_FALSE(ui.empty());
  EXPECT_EQ(count_screens(ui, "Type"), 1u);
  // The differing field is still shown once per message.
  EXPECT_EQ(count_screens(ui, "Validator"), 3u);
}

// cosmos-sdk/MsgDelegate is an immediate x/staking delegation;
// /babylon.epoching.v1.MsgWrappedDelegate is queued to an epoch boundary by
// x/epoching. Both render as "Delegate", so collapsing on the rendered value
// would show a single Type screen for two different operations.
TEST(Grouping, TypesSharingALabelDoNotCollapse) {
  app_mode_set_expert(false);

  const auto ui = render(signdoc({
      staking_msg("cosmos-sdk/MsgDelegate", "cosmosvaloper1honest"),
      staking_msg("/babylon.epoching.v1.MsgWrappedDelegate", "cosmosvaloper1other"),
  }));

  ASSERT_FALSE(ui.empty());
  EXPECT_EQ(count_screens(ui, "Type"), 2u)
      << "two distinct amino types collapsed into one Type screen";
}

TEST(Grouping, LegacyAndProtoSpellingsOfTheSameTypeDoNotCollapse) {
  app_mode_set_expert(false);

  const auto ui = render(signdoc({
      staking_msg("epoching/WrappedDelegate", "cosmosvaloper1honest"),
      staking_msg("/babylon.epoching.v1.MsgWrappedDelegate", "cosmosvaloper1other"),
  }));

  ASSERT_FALSE(ui.empty());
  EXPECT_EQ(count_screens(ui, "Type"), 2u);
}

TEST(Grouping, DistinctTypesWithDistinctLabelsStillDoNotCollapse) {
  app_mode_set_expert(false);

  const auto ui = render(signdoc({
      staking_msg("cosmos-sdk/MsgDelegate", "cosmosvaloper1aaa"),
      staking_msg("cosmos-sdk/MsgUndelegate", "cosmosvaloper1bbb"),
  }));

  ASSERT_FALSE(ui.empty());
  EXPECT_EQ(count_screens(ui, "Type"), 2u);
}
