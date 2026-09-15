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

// The signed bytes are JSON; the chain acts on what those bytes decode to. A
// review that pages the raw span shows something else -- "cosmos1abcAdef"
// reads out with a literal backslash-u while the chain sends to "cosmos1abcAdef".
// Escapes are not exotic either: Go's encoder emits > for '>' by default,
// so ordinary documents carry them.
namespace {

// memo is inserted into the document verbatim, so a test writes the escape
// exactly as it appears in the signed bytes.
std::string signdoc_memo(const std::string &memo) {
  return R"({"account_number":"8","chain_id":"cosmoshub-4",)"
         R"("fee":{"amount":[{"amount":"5000","denom":"uatom"}],"gas":"200000"},)"
         R"("memo":")" + memo + R"(","msgs":[{"type":"cosmos-sdk/MsgSend","value":{)"
         R"("amount":[{"amount":"10","denom":"uatom"}],)"
         R"("from_address":"cosmos1from","to_address":"cosmos1to"}}],"sequence":"1"})";
}

std::string signdoc_to(const std::string &to) {
  return R"({"account_number":"8","chain_id":"cosmoshub-4",)"
         R"("fee":{"amount":[{"amount":"5000","denom":"uatom"}],"gas":"200000"},)"
         R"("memo":"","msgs":[{"type":"cosmos-sdk/MsgSend","value":{)"
         R"("amount":[{"amount":"10","denom":"uatom"}],)"
         R"("from_address":"cosmos1from","to_address":")" + to + R"("}}],"sequence":"1"})";
}

struct Item {
  parser_error_t err;
  std::string key;
  std::string value;
};

// Renders the first item whose key contains `want`.
Item render_field(const std::string &tx, const std::string &want) {
  parser_context_t ctx;
  parser_tx_t tx_obj;
  memset(&tx_obj, 0, sizeof(tx_obj));
  tx_obj.tx_type = tx_json;

  Item item{};
  item.err = parser_parse(&ctx, (const uint8_t *)tx.c_str(), tx.size(), &tx_obj);
  if (item.err != parser_ok) {
    return item;
  }
  item.err = parser_validate(&ctx);
  if (item.err != parser_ok) {
    return item;
  }

  uint8_t numItems = 0;
  item.err = parser_getNumItems(&ctx, &numItems);
  if (item.err != parser_ok) {
    return item;
  }

  for (uint8_t i = 0; i < numItems; i++) {
    char key[128] = {0};
    char val[192] = {0};
    uint8_t pageCount = 0;
    const auto err = parser_getItem(&ctx, i, key, sizeof(key), val, sizeof(val),
                                    0, &pageCount);
    if (err != parser_ok) {
      item.err = err;
      return item;
    }
    if (std::string(key).find(want) != std::string::npos) {
      item.key = key;
      item.value = val;
      return item;
    }
  }
  item.err = parser_no_data;
  return item;
}

}  // namespace

// The finding, end to end: what the review reads has to be what the chain acts
// on.
TEST(JsonEscapes, UnicodeEscapeInAnAddressIsShownDecoded) {
  const auto item = render_field(signdoc_to(R"(cosmos1abc\u0041def)"), "To");
  ASSERT_EQ(item.err, parser_ok);
  EXPECT_EQ(item.value, "cosmos1abcAdef");
}

TEST(JsonEscapes, EscapedSolidusIsShownDecoded) {
  const auto item = render_field(signdoc_memo(R"(a\/b)"), "Memo");
  ASSERT_EQ(item.err, parser_ok);
  EXPECT_EQ(item.value, "a/b");
}

TEST(JsonEscapes, EscapedQuoteAndBackslashAreShownDecoded) {
  const auto item = render_field(signdoc_memo(R"(say \"hi\" \\ done)"), "Memo");
  ASSERT_EQ(item.err, parser_ok);
  EXPECT_EQ(item.value, R"(say "hi" \ done)");
}

// Go escapes '>' by default, so this is what ordinary documents look like.
TEST(JsonEscapes, GoStyleHtmlEscapesAreShownDecoded) {
  const auto item = render_field(signdoc_memo(R"(\u003ehello\u003c)"), "Memo");
  ASSERT_EQ(item.err, parser_ok);
  EXPECT_EQ(item.value, ">hello<");
}

TEST(JsonEscapes, NonAsciiEscapeBecomesUtf8) {
  // U+269B, three bytes once decoded.
  const auto item = render_field(signdoc_memo(R"(\u269b)"), "Memo");
  ASSERT_EQ(item.err, parser_ok);
  EXPECT_EQ(item.value, "\xE2\x9A\x9B");
}

TEST(JsonEscapes, SurrogatePairBecomesUtf8) {
  // U+1F600, four bytes once decoded.
  const auto item = render_field(signdoc_memo(R"(\ud83d\ude00)"), "Memo");
  ASSERT_EQ(item.err, parser_ok);
  EXPECT_EQ(item.value, "\xF0\x9F\x98\x80");
}

// --- forms that must not reach the screen ---

// The control-byte rule inspects raw bytes, which are all printable here. The
// escaped spelling used to walk straight past it and decode to a carriage
// return on the chain.
TEST(JsonEscapes, EscapedCarriageReturnIsRejected) {
  const auto item = render_field(signdoc_memo(R"(SAFE\rDRAINED)"), "Memo");
  EXPECT_EQ(item.err, parser_unexpected_characters);
}

TEST(JsonEscapes, EscapedNewlineIsRejected) {
  const auto item = render_field(signdoc_memo(R"(first\nsecond)"), "Memo");
  EXPECT_EQ(item.err, parser_unexpected_characters);
}

TEST(JsonEscapes, UnicodeEscapedControlByteIsRejected) {
  const auto item = render_field(signdoc_memo(R"(a\u0009b)"), "Memo");
  EXPECT_EQ(item.err, parser_unexpected_characters);
}

TEST(JsonEscapes, UnknownEscapeIsRejected) {
  const auto item = render_field(signdoc_memo(R"(a\qb)"), "Memo");
  EXPECT_EQ(item.err, parser_unexpected_characters);
}

TEST(JsonEscapes, TruncatedUnicodeEscapeIsRejected) {
  const auto item = render_field(signdoc_memo(R"(a\u26)"), "Memo");
  EXPECT_EQ(item.err, parser_unexpected_characters);
}

TEST(JsonEscapes, NonHexUnicodeEscapeIsRejected) {
  const auto item = render_field(signdoc_memo(R"(a\uZZZZb)"), "Memo");
  EXPECT_EQ(item.err, parser_unexpected_characters);
}

TEST(JsonEscapes, LoneHighSurrogateIsRejected) {
  const auto item = render_field(signdoc_memo(R"(\ud83d)"), "Memo");
  EXPECT_EQ(item.err, parser_unexpected_characters);
}

TEST(JsonEscapes, LoneLowSurrogateIsRejected) {
  const auto item = render_field(signdoc_memo(R"(\ude00)"), "Memo");
  EXPECT_EQ(item.err, parser_unexpected_characters);
}

// A backslash cannot be the last byte of a well-formed JSON string -- it would
// escape the closing quote -- so this never reaches the decoder's own guard for
// it. The document is refused before that, which is the outcome that matters.
TEST(JsonEscapes, TrailingBackslashIsRejected) {
  const auto item = render_field(signdoc_memo(R"(abc\)"), "Memo");
  EXPECT_NE(item.err, parser_ok);
}
