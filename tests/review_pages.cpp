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

// Items and review pages are separate budgets. One display item can render many
// pages: parser_formatAmount() sums a page per coin in an amount array into a
// single item's pageCount, and a long value paginates by its length.
//
// This matters because the review counts pages, not items. NBGL addresses pairs
// through a uint8_t, so a document whose pages total past 255 cannot be shown in
// full; zxlib refuses such a document rather than wrapping the count. That bound
// lives in the UI layer, where the real render width is known, so it is not
// asserted here. What is asserted here is the property that makes an item cap
// insufficient on its own, so it does not quietly stop being true.
namespace {

// Every coin in an amount array adds a page to the SAME display item.
std::string coin_array(size_t coins) {
  std::string out = "[";
  for (size_t i = 0; i < coins; i++) {
    if (i > 0) {
      out += ",";
    }
    out += R"({"amount":"1","denom":"d)" + std::to_string(100000 + i) + R"("})";
  }
  return out + "]";
}

std::string document(size_t coins) {
  return R"({"account_number":"8","chain_id":"cosmoshub-4",)"
         R"("fee":{"amount":)" +
         coin_array(coins) + R"(,"gas":"200000"},)" +
         R"("memo":"","msgs":[{"type":"cosmos-sdk/MsgSend","value":{)"
         R"("from_address":"cosmos1from","to_address":"cosmos1to"}}],)"
         R"("sequence":"1"})";
}

}  // namespace

TEST(ReviewPages, OneAmountArrayItemCarriesManyPages) {
  app_mode_set_expert(false);

  const std::string tx = document(60);

  parser_context_t ctx;
  parser_tx_t tx_obj;
  memset(&tx_obj, 0, sizeof(tx_obj));
  tx_obj.tx_type = tx_json;

  ASSERT_EQ(parser_parse(&ctx, (const uint8_t *)tx.c_str(), tx.size(), &tx_obj),
            parser_ok);
  ASSERT_EQ(parser_validate(&ctx), parser_ok);

  uint8_t num_items = 0;
  ASSERT_EQ(parser_getNumItems(&ctx, &num_items), parser_ok);

  char key[64];
  char val[300];
  uint32_t total_pages = 0;
  for (uint8_t idx = 0; idx < num_items; idx++) {
    uint8_t page_count = 0;
    ASSERT_EQ(parser_getItem(&ctx, idx, key, sizeof(key), val, sizeof(val), 0,
                             &page_count),
              parser_ok);
    total_pages += page_count;
  }

  EXPECT_LE(num_items, (uint8_t)MAX_REVIEW_ITEMS);
  EXPECT_GT(total_pages, (uint32_t)num_items)
      << "a 60-coin amount array should make pages outnumber items, which is "
         "why bounding items does not bound the review";
}
