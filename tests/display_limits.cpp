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
#include "tx_display.h"
#include <string>

// The review UI addresses items through zxlib's viewfunc_getItem_t, whose
// displayIdx is int8_t. The parser can render more items than that index can
// reach, so the item count has to stop where the screen does.
namespace {

// Distinct delegator and validator per message, so neither the msg-type nor the
// msg-from grouping collapses screens and the item count grows with the batch.
std::string batch(size_t messages) {
  std::string msgs;
  for (size_t i = 0; i < messages; i++) {
    if (i > 0) {
      msgs += ",";
    }
    const std::string n = std::to_string(100000 + i);
    msgs += R"({"type":"cosmos-sdk/MsgWithdrawDelegationReward","value":{)"
            R"("delegator_address":"cosmos1delegator)" + n + R"(",)"
            R"("validator_address":"cosmosvaloper1validator)" + n + R"("}})";
  }
  return R"({"account_number":"8","chain_id":"cosmoshub-4",)"
         R"("fee":{"amount":[{"amount":"5000","denom":"uatom"}],"gas":"200000"},)"
         R"("memo":"","msgs":[)" + msgs + R"(],"sequence":"1"})";
}

}  // namespace

TEST(DisplayLimits, EveryCountedItemIsReachableThroughTheUiIndex) {
  app_mode_set_expert(false);

  bool saw_accepted = false;
  bool saw_rejected = false;

  for (size_t messages = 1; messages <= 80; messages++) {
    const std::string tx = batch(messages);

    parser_context_t ctx;
    parser_tx_t tx_obj;
    memset(&tx_obj, 0, sizeof(tx_obj));
    tx_obj.tx_type = tx_json;

    auto err = parser_parse(&ctx, (const uint8_t *)tx.c_str(), tx.size(), &tx_obj);
    if (err == parser_ok) {
      err = parser_validate(&ctx);
    }

    if (err != parser_ok) {
      // Growing the batch may exhaust the token pool before the item count is
      // reached; only the item-count rejection is asserted on here.
      if (err == parser_unexpected_number_items) {
        saw_rejected = true;
      }
      continue;
    }

    saw_accepted = true;

    uint8_t numItems = 0;
    ASSERT_EQ(parser_getNumItems(&ctx, &numItems), parser_ok);
    EXPECT_LE(numItems, MAX_REVIEW_ITEMS) << messages << " messages";

    // The invariant that matters: an accepted transaction must not count an
    // item the UI cannot ask for.
    for (uint8_t i = 0; i < numItems; i++) {
      ASSERT_GE((int8_t)i, 0) << "item " << (int)i << " of " << (int)numItems
                              << " is unreachable through an int8_t index ("
                              << messages << " messages)";
    }
  }

  EXPECT_TRUE(saw_accepted) << "no batch was accepted; the fixture is wrong";
  EXPECT_TRUE(saw_rejected) << "no batch exceeded the item limit; widen the loop";
}

TEST(DisplayLimits, OversizedBatchIsRejectedNotTruncated) {
  app_mode_set_expert(false);

  const std::string tx = batch(70);

  parser_context_t ctx;
  parser_tx_t tx_obj;
  memset(&tx_obj, 0, sizeof(tx_obj));
  tx_obj.tx_type = tx_json;

  auto err = parser_parse(&ctx, (const uint8_t *)tx.c_str(), tx.size(), &tx_obj);
  if (err == parser_ok) {
    err = parser_validate(&ctx);
  }

  // A batch this size cannot be reviewed in full, so it must not be signed at
  // all rather than shown up to the point the index runs out.
  EXPECT_NE(err, parser_ok);
}
