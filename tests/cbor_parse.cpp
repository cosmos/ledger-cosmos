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

parser_error_t validate_textual(const std::vector<uint8_t> &blob) {
  parser_context_t ctx;
  parser_tx_t tx_obj;
  memset(&tx_obj, 0, sizeof(tx_obj));
  tx_obj.tx_type = tx_textual;
  const parser_error_t err = parser_parse(&ctx, blob.data(), blob.size(), &tx_obj);
  if (err != parser_ok) {
    return err;
  }
  return parser_validate(&ctx);
}

// A definite-length CBOR text string, the only string shape the parser takes.
std::vector<uint8_t> text_string(const std::string &value) {
  std::vector<uint8_t> out;
  if (value.size() < 24) {
    out.push_back(static_cast<uint8_t>(0x60 | value.size()));
  } else if (value.size() <= 0xff) {
    out.push_back(0x78);
    out.push_back(static_cast<uint8_t>(value.size()));
  } else {
    out.push_back(0x79);
    out.push_back(static_cast<uint8_t>(value.size() >> 8));
    out.push_back(static_cast<uint8_t>(value.size() & 0xff));
  }
  out.insert(out.end(), value.begin(), value.end());
  return out;
}

std::vector<uint8_t> titled_screen(const std::string &title, const std::string &content) {
  std::vector<uint8_t> out = {0xa2, 0x01};
  const auto t = text_string(title);
  out.insert(out.end(), t.begin(), t.end());
  out.push_back(0x02);
  const auto c = text_string(content);
  out.insert(out.end(), c.begin(), c.end());
  return out;
}

std::vector<uint8_t> untitled_screen(const std::string &content) {
  std::vector<uint8_t> out = {0xa1, 0x02};
  const auto c = text_string(content);
  out.insert(out.end(), c.begin(), c.end());
  return out;
}

// { 1: title, 2: content, 3: indent } -- the indent the renderer turns into a
// run of SCREEN_INDENT characters in front of the key.
std::vector<uint8_t> indented_screen(const std::string &title, const std::string &content, uint16_t indent) {
  std::vector<uint8_t> out = {0xa3, 0x01};
  const auto t = text_string(title);
  out.insert(out.end(), t.begin(), t.end());
  out.push_back(0x02);
  const auto c = text_string(content);
  out.insert(out.end(), c.begin(), c.end());
  out.push_back(0x03);
  if (indent < 24) {
    out.push_back(static_cast<uint8_t>(indent));
  } else {
    out.push_back(0x18);
    out.push_back(static_cast<uint8_t>(indent));
  }
  return out;
}

// { 2: content, 3: indent, 4: expert } -- an untitled screen carrying an
// indent. The expert flag is what makes the field count exceed the two the
// screen walk subtracts, so the optional-field pass reaches the indent at all.
std::vector<uint8_t> indented_untitled_screen(const std::string &content, uint16_t indent) {
  std::vector<uint8_t> out = {0xa3, 0x02};
  const auto c = text_string(content);
  out.insert(out.end(), c.begin(), c.end());
  out.push_back(0x03);
  if (indent < 24) {
    out.push_back(static_cast<uint8_t>(indent));
  } else {
    out.push_back(0x18);
    out.push_back(static_cast<uint8_t>(indent));
  }
  out.insert(out.end(), {0x04, 0xf5});
  return out;
}

std::vector<uint8_t> envelope(const std::vector<std::vector<uint8_t>> &screens) {
  std::vector<uint8_t> out = {0xa1, 0x01};
  if (screens.size() < 24) {
    out.push_back(static_cast<uint8_t>(0x80 | screens.size()));
  } else {
    out.push_back(0x98);
    out.push_back(static_cast<uint8_t>(screens.size()));
  }
  for (const auto &screen : screens) {
    out.insert(out.end(), screen.begin(), screen.end());
  }
  return out;
}

struct Rendered {
  parser_error_t err;
  std::string key;
  std::string value;
};

// Parses a document and renders one screen the way the review UI does. The
// blob has to outlive the call: textual screens are pointers into it.
Rendered render_textual(const std::vector<uint8_t> &blob, uint8_t displayIdx) {
  parser_context_t ctx;
  parser_tx_t tx_obj;
  memset(&tx_obj, 0, sizeof(tx_obj));
  tx_obj.tx_type = tx_textual;

  Rendered rendered{};
  rendered.err = parser_parse(&ctx, blob.data(), blob.size(), &tx_obj);
  if (rendered.err != parser_ok) {
    return rendered;
  }

  char outKey[64] = {0};
  char outVal[64] = {0};
  uint8_t pageCount = 0;
  rendered.err = parser_getItem(&ctx, displayIdx, outKey, sizeof(outKey), outVal, sizeof(outVal), 0, &pageCount);
  rendered.key = outKey;
  rendered.value = outVal;
  return rendered;
}

}  // namespace

// Textual deliberately leaves the screens to the host: the device renders what
// it is given rather than deciding what a transaction ought to look like. The
// wording has already moved -- the message count screen reads "Transaction: 1
// Messages" in TXSPEC, "This transaction has 1 Message" in what ships, and
// "This transaction has <int> Message(s)" in ADR-050 -- so a document is not
// held to any particular envelope, only to the encoding.
TEST(CborTextual, MinimalScreenIsAccepted) {
  // { 1: "t", 2: "c" }
  const auto blob = document({0xa2, 0x01, 0x61, 't', 0x02, 0x61, 'c'}, 1);
  EXPECT_EQ(parse_textual(blob), parser_ok);
  EXPECT_EQ(validate_textual(blob), parser_ok);
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

// The review UI reaches screens through an int8_t index, so a document with
// more screens than MAX_REVIEW_ITEMS would be reviewed only up to screen 127
// while the signature still covers all of it. Refuse the whole document.
TEST(CborTextual, ScreenCountAtTheCeilingIsAccepted) {
  std::vector<std::vector<uint8_t>> screens;
  for (size_t i = 0; i < 127; i++) {
    screens.push_back(titled_screen("t", "c"));
  }
  EXPECT_EQ(parse_textual(envelope(screens)), parser_ok);
}

TEST(CborTextual, ScreenCountAboveTheCeilingIsRejected) {
  for (const size_t count : {128u, 200u, 255u}) {
    std::vector<std::vector<uint8_t>> screens;
    for (size_t i = 0; i < count; i++) {
      screens.push_back(titled_screen("t", "c"));
    }
    EXPECT_EQ(parse_textual(envelope(screens)), parser_unexpected_number_items)
        << count << " screens were accepted";
  }
}

// The one thing a host cannot ask for: a screen carrying nothing. TXSPEC has
// empty entries omitted from the encoding, not sent, and a blank page shows
// the user none of the document while still standing between them and the
// approval.
TEST(CborTextual, BlankScreenIsRejected) {
  const auto blob = envelope({titled_screen("Chain id", "my-chain"),
                              titled_screen("Memo", "")});
  EXPECT_EQ(validate_textual(blob), parser_unexpected_value);
}

// The payload the Zemu textual tests sign, byte for byte: a real rendering has
// to keep passing.
TEST(CborTextual, ZemuFixtureIsAccepted) {
  const std::string hex =
      "a10192a20168436861696e20696402686d792d636861696ea2016e4163636f756e74206e756d626572026131a2016853"
      "657175656e6365026132a301674164647265737302782d636f736d6f7331756c6176336873656e7570737771666b7732"
      "7933737570356b677471776e767161386579687304f5a3016a5075626c6963206b657902781f2f636f736d6f732e6372"
      "7970746f2e736563703235366b312e5075624b657904f5a3026d5075624b6579206f626a656374030104f5a401634b65"
      "790278523032454220444437462045344644204542373620444338412032303545204636354420373930432044333045"
      "2038413337203541354320323532382045423341203932334120463146422034443739203444030204f5a102781e5468"
      "6973207472616e73616374696f6e206861732031204d657373616765a3016d4d6573736167652028312f312902781c2f"
      "636f736d6f732e62616e6b2e763162657461312e4d736753656e640301a2026e4d736753656e64206f626a6563740302"
      "a3016c46726f6d206164647265737302782d636f736d6f7331756c6176336873656e7570737771666b77327933737570"
      "356b677471776e76716138657968730303a3016a546f206164647265737302782d636f736d6f7331656a726634637572"
      "327779366b667572673966326a707070326833616665356836706b6835740303a30166416d6f756e7402673130204154"
      "4f4d0303a1026e456e64206f66204d657373616765a201644d656d6f0278193e20e29a9befb88f5c7532363942e29a9b"
      "efb88f2020202020a2016446656573026a302e3030322041544f4da30169476173206c696d6974026731303027303030"
      "04f5a3017148617368206f66207261772062797465730278403963303433323930313039633237306232666661396633"
      "633066613535613039306330313235656265663838316637646135333937386462663933663733383504f5";

  std::vector<uint8_t> blob;
  for (size_t i = 0; i + 1 < hex.size(); i += 2) {
    blob.push_back(static_cast<uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16)));
  }
  EXPECT_EQ(validate_textual(blob), parser_ok);
}
// A screen key wide enough to overflow an int used to be read with the
// truncating getter, so 2^32 + 1 arrived as 1 and was dispatched as TITLE. The
// device would then render a screen a standards-compliant reader would not,
// while the signature still covered the original document.
TEST(CborTextual, OversizedScreenKeyIsRejected) {
  // { 1: [ { 0x100000001: "t", 2: "c" } ] }
  const auto blob =
      document({0xa2, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x61, 't', 0x02, 0x61, 'c'}, 1);
  EXPECT_NE(parse_textual(blob), parser_ok);
}

// Same truncation on the outer envelope key: 2^32 + 1 must not pass for the 1
// the document is required to carry.
TEST(CborTextual, OversizedEnvelopeKeyIsRejected) {
  std::vector<uint8_t> blob = {0xa1, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x81};
  const std::vector<uint8_t> screen = {0xa2, 0x01, 0x61, 't', 0x02, 0x61, 'c'};
  blob.insert(blob.end(), screen.begin(), screen.end());
  EXPECT_NE(parse_textual(blob), parser_ok);
}

// An oversized optional key has the same problem one level down: 2^32 + 3 must
// not be taken for INDENT.
TEST(CborTextual, OversizedOptionalKeyIsRejected) {
  // { 1: [ { 1: "t", 2: "c", 0x100000003: 2 } ] }
  const auto blob = document({0xa3, 0x01, 0x61, 't', 0x02, 0x61, 'c', 0x1b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
                              0x03, 0x02},
                             1);
  EXPECT_NE(parse_textual(blob), parser_ok);
}

// Every element of the screen array is read with cbor_value_get_map_length,
// which only asserts that it was handed a map -- and asserts are compiled out
// of production builds. An element of any other type has to be a clean parsing
// error rather than a length read off a value that has none.
TEST(CborTextual, NonMapScreenIsRejected) {
  // { 1: [ 5 ] }
  const auto blob = document({0x05}, 1);
  EXPECT_EQ(parse_textual(blob), parser_unexpected_type);
}

TEST(CborTextual, ArrayScreenIsRejected) {
  // { 1: [ [ "t" ] ] }
  const auto blob = document({0x81, 0x61, 't'}, 1);
  EXPECT_EQ(parse_textual(blob), parser_unexpected_type);
}

TEST(CborTextual, TextScreenIsRejected) {
  // { 1: [ "screen" ] }
  const auto blob = document({0x66, 's', 'c', 'r', 'e', 'e', 'n'}, 1);
  EXPECT_EQ(parse_textual(blob), parser_unexpected_type);
}

// A screen's indent is host-controlled and only bounded by UINT8_MAX, while
// the key it prefixes is assembled in a MAX_TITLE_SIZE + 2 buffer. When
// z_str3join runs out of room it does not simply stop: it overwrites what it
// was given with the literal "ERR???" and reports the failure. Dropping that
// return value put the marker on the review screen in place of the title,
// while the content it belonged to -- and the bytes being signed -- stayed
// exactly as the host sent them. There is no title a user can check against
// "ERR???", so the request has to end instead.
TEST(CborTextualRender, IndentThatOverflowsTheTruncatedKeyEndsTheRequest) {
  // titleLen 1 + indent 39 clears PRINTABLE_TITLE_SIZE, so the key is the
  // "---" truncation marker plus one '>' per indent level: 3 + 39 leaves no
  // room for the terminator in a 42-byte buffer.
  const auto blob = envelope({indented_screen("t", "content", 39)});
  const auto rendered = render_textual(blob, 0);

  EXPECT_EQ(rendered.err, parser_unexpected_buffer_end);
  EXPECT_THAT(rendered.key, testing::Not(testing::HasSubstr("ERR")));
}

TEST(CborTextualRender, IndentJustBelowTheTruncatedKeyLimitStillRenders) {
  const auto blob = envelope({indented_screen("t", "content", 38)});
  const auto rendered = render_textual(blob, 0);

  EXPECT_EQ(rendered.err, parser_ok);
  EXPECT_EQ(rendered.key, std::string(38, '>') + "---");
}

// The same buffer, reached the other way: parser_screenPrint adds titleLen and
// indent into a uint8_t to pick between the truncated and the normal key. A
// title at MAX_TITLE_SIZE with indent 216 sums to 256, which wraps to 0 and
// takes the normal branch -- where the full 40-character title is already in
// the buffer before the first '>' is prepended.
TEST(CborTextualRender, IndentThatWrapsIntoTheNormalKeyEndsTheRequest) {
  const auto blob = envelope({indented_screen(std::string(40, 'T'), "content", 216)});
  const auto rendered = render_textual(blob, 0);

  EXPECT_EQ(rendered.err, parser_unexpected_buffer_end);
  EXPECT_THAT(rendered.key, testing::Not(testing::HasSubstr("ERR")));
}

// An untitled screen indents the value instead of the key, in the
// OUTPUT_HANDLER_SIZE buffer the translated content already occupies.
TEST(CborTextualRender, IndentThatOverflowsAnUntitledValueEndsTheRequest) {
  const auto blob = envelope({indented_untitled_screen(std::string(MAX_CONTENT_SIZE, 'c'), 60)});
  const auto rendered = render_textual(blob, 0);

  EXPECT_EQ(rendered.err, parser_unexpected_buffer_end);
  EXPECT_THAT(rendered.value, testing::Not(testing::HasSubstr("ERR")));
}

TEST(CborTextualRender, ModestIndentRendersNormally) {
  const auto blob = envelope({indented_screen("Amount", "10 ATOM", 2)});
  const auto rendered = render_textual(blob, 0);

  EXPECT_EQ(rendered.err, parser_ok);
  EXPECT_EQ(rendered.key, ">>Amount");
  EXPECT_EQ(rendered.value, "10 ATOM");
}
