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
#include <vector>

// A SIGN_MODE_TEXTUAL payload is a CBOR map { 1: [ screen, ... ] } where every
// screen is a map keyed by TITLE(1) / CONTENT(2) / INDENT(3) / EXPERT(4). The
// whole document comes from the host, so the parser has to be strict about
// shapes it will never render.
namespace {

// Wraps screens into the outer `{ 1: [ ... ] }` document.
std::vector<uint8_t> document(const std::vector<uint8_t> &screens, uint8_t count) {
  std::vector<uint8_t> out = {0xa1, 0x01, static_cast<uint8_t>(0x80 | count)};
  out.insert(out.end(), screens.begin(), screens.end());
  return out;
}

parser_error_t parse_textual(const std::vector<uint8_t> &blob) {
  parser_context_t ctx;
  parser_tx_t tx_obj;
  memset(&tx_obj, 0, sizeof(tx_obj));
  tx_obj.tx_type = tx_textual;
  return parser_parse(&ctx, blob.data(), blob.size(), &tx_obj);
}

}  // namespace

TEST(CborTextual, MinimalScreenIsAccepted) {
  // { 1: "t", 2: "c" }
  const auto blob = document({0xa2, 0x01, 0x61, 't', 0x02, 0x61, 'c'}, 1);
  EXPECT_EQ(parse_textual(blob), parser_ok);
}

TEST(CborTextual, EverySchemaFieldIsAccepted) {
  // { 1: "t", 2: "c", 3: 2, 4: true }
  const auto blob = document({0xa4, 0x01, 0x61, 't', 0x02, 0x61, 'c', 0x03, 0x02, 0x04, 0xf5}, 1);
  EXPECT_EQ(parse_textual(blob), parser_ok);
}

TEST(CborTextual, UnknownKeyIsRejected) {
  // { 1: "t", 2: "c", 9: "x" } -- key 9 is outside the schema. Skipping it
  // would step over a value of unknown shape and clear options already set.
  const auto blob = document({0xa3, 0x01, 0x61, 't', 0x02, 0x61, 'c', 0x09, 0x61, 'x'}, 1);
  EXPECT_EQ(parse_textual(blob), parser_unexpected_field);
}

TEST(CborTextual, IndefiniteLengthTitleIsRejected) {
  // { 1: (_ "t"), 2: "c" } -- an indefinite-length string reports itself as a
  // text string but cannot be consumed as one chunk.
  const auto blob = document({0xa2, 0x01, 0x7f, 0x61, 't', 0xff, 0x02, 0x61, 'c'}, 1);
  EXPECT_EQ(parse_textual(blob), parser_unexpected_type);
}

TEST(CborTextual, ContainerAsTitleIsRejected) {
  // { 1: ["t"], 2: "c" } -- a title that is an array rather than a string.
  const auto blob = document({0xa2, 0x01, 0x81, 0x61, 't', 0x02, 0x61, 'c'}, 1);
  EXPECT_EQ(parse_textual(blob), parser_unexpected_type);
}

// The regression this file exists for: cbor_value_advance() recurses once per
// container nesting level, so a title built from deeply nested arrays used to
// drive the parser hundreds of frames deep on a device whose whole stack is
// APP_STACK_MIN_SIZE. Rejection must happen on shape, before any descent.
TEST(CborTextual, DeeplyNestedTitleIsRejectedWithoutDescending) {
  for (const size_t depth : {8u, 64u, 512u, 900u}) {
    std::vector<uint8_t> screen = {0xa2, 0x01};
    screen.insert(screen.end(), depth, 0x81);  // depth x array(1)
    screen.push_back(0x60);                    // innermost value: ""
    screen.insert(screen.end(), {0x02, 0x61, 'x'});

    const auto err = parse_textual(document(screen, 1));
    EXPECT_NE(err, parser_ok) << "depth " << depth << " was accepted";
    EXPECT_EQ(err, parser_unexpected_type) << "depth " << depth;
  }
}

TEST(CborTextual, DeeplyNestedContentIsRejectedWithoutDescending) {
  std::vector<uint8_t> screen = {0xa2, 0x01, 0x61, 't', 0x02};
  screen.insert(screen.end(), 900, 0x81);
  screen.push_back(0x60);

  EXPECT_EQ(parse_textual(document(screen, 1)), parser_unexpected_type);
}

TEST(CborTextual, TrailingBytesAreRejected) {
  auto blob = document({0xa2, 0x01, 0x61, 't', 0x02, 0x61, 'c'}, 1);
  blob.insert(blob.end(), {0xde, 0xad, 0xbe, 0xef});
  EXPECT_NE(parse_textual(blob), parser_ok);
}
