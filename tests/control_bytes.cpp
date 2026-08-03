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

// A canonical SignDoc is the compact form sdk.MustSortJSON emits, so no raw
// control byte ever appears in one. They matter because the whitespace check
// only inspects the gaps between tokens, never a token's own span, and the
// JSON display path renders token bytes verbatim -- so a control byte inside a
// string value is signed but can be pushed off the review page.
namespace {

std::string signdoc(const std::string &memo) {
  return R"({"account_number":"8","chain_id":"cosmoshub-4",)"
         R"("fee":{"amount":[{"amount":"5000","denom":"uatom"}],"gas":"200000"},)"
         R"("memo":")" + memo + R"(","msgs":[{"type":"cosmos-sdk/MsgSend","value":{)"
         R"("amount":[{"amount":"10","denom":"uatom"}],)"
         R"("from_address":"cosmos1from","to_address":"cosmos1to"}}],"sequence":"1"})";
}

parser_error_t parse(const std::string &tx) {
  parser_context_t ctx;
  parser_tx_t tx_obj;
  memset(&tx_obj, 0, sizeof(tx_obj));
  tx_obj.tx_type = tx_json;

  const auto err =
      parser_parse(&ctx, (const uint8_t *)tx.c_str(), tx.size(), &tx_obj);
  if (err != parser_ok) {
    return err;
  }
  return parser_validate(&ctx);
}

}  // namespace

TEST(ControlBytes, CanonicalDocumentIsAccepted) {
  EXPECT_EQ(parse(signdoc("Zondax.ch")), parser_ok);
}

// 0x20 is the boundary: a space inside a string value is ordinary content and
// lives in a token's span, where the whitespace check does not reach. It has
// always been accepted and must stay that way.
TEST(ControlBytes, SpaceInsideAStringValueIsStillAccepted) {
  EXPECT_EQ(parse(signdoc("hello world")), parser_ok);
}

TEST(ControlBytes, RawNewlineInsideAStringIsRejected) {
  EXPECT_EQ(parse(signdoc("first\nsecond")), parser_unexpected_characters);
}

TEST(ControlBytes, RawTabInsideAStringIsRejected) {
  EXPECT_EQ(parse(signdoc("a\tb")), parser_unexpected_characters);
}

TEST(ControlBytes, RawCarriageReturnInsideAStringIsRejected) {
  // The classic overwrite trick: a terminal or renderer honouring CR shows only
  // what follows it.
  EXPECT_EQ(parse(signdoc("SAFE LOOKING MEMO\rDRAINED")), parser_unexpected_characters);
}

TEST(ControlBytes, EveryC0ByteIsRejected) {
  for (int c = 0x01; c <= 0x1f; c++) {
    const std::string memo = std::string("a") + static_cast<char>(c) + "b";
    EXPECT_EQ(parse(signdoc(memo)), parser_unexpected_characters)
        << "byte 0x" << std::hex << c << " was accepted";
  }
}

TEST(ControlBytes, EmbeddedNulIsStillRejected) {
  // Pre-existing guard: NUL also lets JSMN stop early on a hidden suffix.
  std::string tx = signdoc("visible");
  tx.insert(tx.size() / 2, std::string(1, '\0'));
  EXPECT_EQ(parse(tx), parser_unexpected_characters);
}

// A memo long enough to paginate, carrying enough newlines that each page would
// demand far more display lines than a review page renders. This is the shape
// that hides signed bytes behind the truncation NBGL applies per page.
TEST(ControlBytes, NewlineFloodedMemoIsRejected) {
  std::string memo;
  for (int i = 0; i < 60; i++) {
    memo += "L" + std::to_string(i) + "\n";
  }
  EXPECT_EQ(parse(signdoc(memo)), parser_unexpected_characters);
}

// Closing the control-byte hole does not address look-alike characters: these
// are legal UTF-8 and legal JSON, and still render verbatim on the JSON path.
// Recorded so the gap is not mistaken for covered.
TEST(ControlBytes, DISABLED_LookalikeCharactersAreStillAccepted) {
  EXPECT_EQ(parse(signdoc("cosmos1‮reversed")), parser_ok);
}
