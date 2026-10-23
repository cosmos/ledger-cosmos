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

// Renders SIGN_MODE_TEXTUAL screens and checks what reaches the review pages.
namespace {

std::vector<uint8_t> text_string(const std::string &v) {
  std::vector<uint8_t> o;
  if (v.size() < 24) {
    o.push_back(0x60 | v.size());
  } else if (v.size() <= 0xff) {
    o.push_back(0x78);
    o.push_back(v.size());
  } else {
    o.push_back(0x79);
    o.push_back(v.size() >> 8);
    o.push_back(v.size() & 0xff);
  }
  o.insert(o.end(), v.begin(), v.end());
  return o;
}

// One screen map: {1: title, 2: content, 3: indent}, keys omitted when empty.
std::vector<uint8_t> screen(const std::string &title, const std::string &content,
                            uint8_t indent) {
  std::vector<uint8_t> o = {
      static_cast<uint8_t>(0xa0 | (1 + !title.empty() + (indent ? 1 : 0)))};
  if (!title.empty()) {
    o.push_back(1);
    const auto t = text_string(title);
    o.insert(o.end(), t.begin(), t.end());
  }
  o.push_back(2);
  const auto c = text_string(content);
  o.insert(o.end(), c.begin(), c.end());
  if (indent) {
    o.push_back(3);
    o.push_back(indent);
  }
  return o;
}

std::vector<uint8_t> envelope(const std::vector<std::vector<uint8_t>> &screens) {
  std::vector<uint8_t> o = {0xa1, 0x01, static_cast<uint8_t>(0x80 | screens.size())};
  for (const auto &s : screens) o.insert(o.end(), s.begin(), s.end());
  return o;
}

struct Rendered {
  parser_error_t err;
  std::string key;
  std::string value;  // all pages joined
};

Rendered render(const std::vector<uint8_t> &blob, uint8_t idx, uint16_t valLen,
                bool expert) {
  app_mode_set_expert(expert);
  parser_context_t ctx;
  parser_tx_t tx_obj;
  memset(&tx_obj, 0, sizeof(tx_obj));
  tx_obj.tx_type = tx_textual;

  Rendered r{};
  r.err = parser_parse(&ctx, blob.data(), blob.size(), &tx_obj);
  if (r.err == parser_ok) r.err = parser_validate(&ctx);

  std::vector<char> key(64), val(valLen);
  uint8_t pageCount = 1;
  for (uint8_t p = 0; r.err == parser_ok && p < pageCount; p++) {
    r.err = parser_getItem(&ctx, idx, key.data(), key.size(), val.data(), valLen,
                           p, &pageCount);
    r.key = key.data();
    r.value += val.data();
  }
  app_mode_set_expert(false);
  return r;
}

// Value buffers the devices pass: Nano S+, Nano X, Flex, Stax, Apex.
const uint16_t kValueLens[] = {4096, 256, 162, 160, 144};

}  // namespace

// x/gov MsgVote metadata at its 255-byte maximum, indent 3, escape-heavy:
// 68 x U+0010 (6 characters each once escaped) and 187 ASCII characters.
// Long titles fold into "---" / "<title>: <content>", which must still carry
// the whole content.
TEST(TextualDisplay, LongTitleLayoutShowsTheWholeContent) {
  std::string content(68, '\x10');
  content += std::string(182, 'm');
  content += "ZZZZZ";
  ASSERT_EQ(content.size(), 255u);
  const auto blob = envelope({screen("Chain id", "cosmoshub-4", 0),
                              screen("", "MsgVote object", 2),
                              screen("Metadata", content, 3)});
  for (const bool expert : {false, true}) {
    for (const uint16_t valLen : kValueLens) {
      const auto r = render(blob, 2, valLen, expert);
      ASSERT_EQ(r.err, parser_ok) << "valLen=" << valLen;
      ASSERT_GE(r.value.size(), 5u);
      EXPECT_EQ(r.value.substr(r.value.size() - 5), "ZZZZZ")
          << "valLen=" << valLen << " expert=" << expert;
    }
  }
}

// The largest case: a 40-character title and a content that translates to
// 599 characters.
TEST(TextualDisplay, LongestTitleAndContentFitTogether) {
  const std::string title(MAX_TITLE_SIZE, 'T');
  std::string content(91, '\x10');  // 546 characters once escaped
  content += std::string(48, 'm');
  content += "ZZZZZ";
  const auto blob = envelope({screen(title, content, 0)});
  for (const uint16_t valLen : kValueLens) {
    const auto r = render(blob, 0, valLen, false);
    ASSERT_EQ(r.err, parser_ok) << "valLen=" << valLen;
    EXPECT_EQ(r.value.size(), title.size() + 2 + 599) << "valLen=" << valLen;
    EXPECT_EQ(r.value.substr(r.value.size() - 5), "ZZZZZ");
  }
}

// ADR-050 Annex 2: control characters other than the named escapes are shown
// as \uXXXX. DEL has no glyph, so drawn raw it would vanish from the value.
TEST(TextualDisplay, DelIsEscaped) {
  const auto r = render(envelope({screen("Memo", std::string("12\x7f" "34"), 0)}),
                        0, 4096, false);
  ASSERT_EQ(r.err, parser_ok);
  EXPECT_EQ(r.value, R"(12\u007F34)");
}

// An untitled screen keeps its indent whether or not it also carries the
// expert key: {2: content, 3: indent} used to lose it.
TEST(TextualDisplay, UntitledScreenKeepsItsIndent) {
  const auto r = render(envelope({screen("", "MsgSend object", 2)}), 0, 4096, false);
  ASSERT_EQ(r.err, parser_ok);
  EXPECT_EQ(r.value, ">>MsgSend object");

  // Same screen with {4: false} after the indent.
  std::vector<uint8_t> withExpert = screen("", "MsgSend object", 2);
  withExpert[0] += 1;
  withExpert.push_back(4);
  withExpert.push_back(0xf4);
  const auto r2 = render(envelope({withExpert}), 0, 4096, false);
  ASSERT_EQ(r2.err, parser_ok);
  EXPECT_EQ(r2.value, r.value);
}
