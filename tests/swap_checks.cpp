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

// Host tests for the Exchange (swap) handlers, compiled from app/src/swap
// against the real parser library (swap_lib in CMakeLists.txt). They drive the
// sequence tx_parse() runs in swap mode -- parser_parse, parser_validate,
// check_swap_conditions -- after copy_transaction_parameters() has stored what
// Exchange approved.
#include <gmock/gmock.h>

#include <cstring>
#include <string>
#include <vector>

#include "app_mode.h"
#include "common/parser.h"
#include "parser_impl.h"

extern "C" {
#include "swap.h"
#include "swap_host_stubs.h"
#include "swap_utils.h"
}

namespace {

const char *kFrom = "cosmos12fuhfs6juf3m47dgplrhdrkvdyn9wc2jmp4sw40";
const char *kDest = "cosmos1s3mu2eyc7ql5hrcqvuerfhxxfxs7ax4kzye8hz";
const char *kOsmoDest = "osmo1s3mu2eyc7ql5hrcqvuerfhxxfxs7ax4kgwq4g6";

std::vector<uint8_t> be(uint64_t v) {
  std::vector<uint8_t> out;
  for (int i = 7; i >= 0; i--) {
    out.push_back((uint8_t)(v >> (8 * i)));
  }
  return out;
}

// What Exchange hands to SIGN_TRANSACTION. coin_configuration is [len][HRP].
bool set_swap(uint64_t amount, uint64_t fee, const std::string &dest,
              const std::string &memo, const std::string &coin_cfg = "cosmos") {
  static std::vector<uint8_t> a, f, cfg;
  static std::string d, m;
  a = be(amount);
  f = be(fee);
  d = dest;
  m = memo;
  cfg.assign(1, (uint8_t)coin_cfg.size());
  cfg.insert(cfg.end(), coin_cfg.begin(), coin_cfg.end());
  create_transaction_parameters_t p;
  memset(&p, 0, sizeof(p));
  p.coin_configuration = cfg.data();
  p.coin_configuration_length = (uint8_t)cfg.size();
  p.amount = a.data();
  p.amount_length = (uint8_t)a.size();
  p.fee_amount = f.data();
  p.fee_amount_length = (uint8_t)f.size();
  p.destination_address = (char *)d.c_str();
  p.destination_address_extra_id = (char *)m.c_str();
  const bool ok = copy_transaction_parameters(&p);
  G_swap_state.called_from_swap = 1;
  G_swap_state.should_exit = 0;
  return ok;
}

std::string coin(const std::string &amount, const std::string &denom) {
  return R"({"amount":")" + amount + R"(","denom":")" + denom + R"("})";
}

std::string send_msg(const std::string &amount_coins, const std::string &to,
                     const std::string &from = kFrom) {
  return R"({"type":"cosmos-sdk/MsgSend","value":{"amount":[)" + amount_coins +
         R"(],"from_address":")" + from + R"(","to_address":")" + to +
         R"("}})";
}

struct Tx {
  std::string chain = "cosmoshub-4";
  std::string fee_coins = coin("5000", "uatom");
  std::string fee_extra; // e.g. ,"granter":"..." (must sort after "gas")
  std::string memo = "12345";
  std::vector<std::string> msgs;
  std::string after_sequence; // e.g. ,"timeout_height":"10"

  std::string json() const {
    std::string s = R"({"account_number":"8","chain_id":")" + chain +
                    R"(","fee":{"amount":[)" + fee_coins +
                    R"(],"gas":"200000")" + fee_extra + R"(},"memo":")" + memo +
                    R"(","msgs":[)";
    for (size_t i = 0; i < msgs.size(); i++) {
      s += (i ? "," : "") + msgs[i];
    }
    s += R"(],"sequence":"1")" + after_sequence + "}";
    return s;
  }
};

parser_error_t run_swap(const std::string &blob, bool expert,
                        tx_type_e type = tx_json) {
  app_mode_set_expert(expert ? 1 : 0);
  parser_tx_obj.tx_json.own_addr = nullptr;
  parser_tx_obj.tx_json.own_addr_len = 0;

  static parser_tx_t tx_obj; // tx.c keeps it static too
  static std::string keep;
  keep = blob;
  parser_context_t ctx;
  memset(&tx_obj, 0, sizeof(tx_obj));
  tx_obj.tx_type = type;
  parser_error_t err = parser_parse(&ctx, (const uint8_t *)keep.data(),
                                    keep.size(), &tx_obj);
  if (err != parser_ok) {
    return err;
  }
  err = parser_validate(&ctx);
  if (err != parser_ok) {
    return err;
  }
  return check_swap_conditions(&ctx);
}

Tx base_tx(const std::string &memo = "12345") {
  Tx t;
  t.memo = memo;
  t.msgs.push_back(send_msg(coin("6000000", "uatom"), kDest));
  return t;
}

// ---- CBOR helpers for a host-authored SIGN_MODE_TEXTUAL document ----------
void cbor_tstr(std::vector<uint8_t> &o, const std::string &s) {
  if (s.size() < 24) {
    o.push_back(0x60 | (uint8_t)s.size());
  } else {
    o.push_back(0x78);
    o.push_back((uint8_t)s.size());
  }
  o.insert(o.end(), s.begin(), s.end());
}

std::string textual(const std::vector<std::pair<std::string, std::string>> &s) {
  std::vector<uint8_t> o = {0xA1, 0x01, (uint8_t)(0x80 | s.size())};
  for (const auto &kv : s) {
    o.push_back(0xA2);
    o.push_back(0x01);
    cbor_tstr(o, kv.first);
    o.push_back(0x02);
    cbor_tstr(o, kv.second);
  }
  return std::string(o.begin(), o.end());
}

class SwapGate : public ::testing::TestWithParam<bool> {
protected:
  void TearDown() override { app_mode_set_expert(0); }
};

} // namespace

// ---------------------------------------------------------------------------
// Baseline: the shapes Exchange is meant to accept.
// ---------------------------------------------------------------------------
TEST_P(SwapGate, BaselineSendWithMemoIsAccepted) {
  ASSERT_TRUE(set_swap(6000000, 5000, kDest, "12345"));
  EXPECT_EQ(run_swap(base_tx().json(), GetParam()), parser_ok);
}

TEST_P(SwapGate, BaselineSendWithoutMemoIsAccepted) {
  ASSERT_TRUE(set_swap(6000000, 5000, kDest, ""));
  EXPECT_EQ(run_swap(base_tx("").json(), GetParam()), parser_ok);
}

TEST_P(SwapGate, OsmosisSendIsAccepted) {
  ASSERT_TRUE(set_swap(6000000, 5000, kOsmoDest, "", "osmo"));
  Tx t;
  t.chain = "osmosis-1";
  t.fee_coins = coin("5000", "uosmo");
  t.memo = "";
  t.msgs.push_back(send_msg(coin("6000000", "uosmo"),
                            kOsmoDest, "osmo12fuhfs6juf3m47dgplrhdrkvdyn9wc2jsvqf5s"));
  EXPECT_EQ(run_swap(t.json(), GetParam()), parser_ok);
}

// ---------------------------------------------------------------------------
// Amount / fee / denom / recipient / memo variants that must be refused.
// ---------------------------------------------------------------------------
TEST_P(SwapGate, SecondCoinInAmountIsRefused) {
  ASSERT_TRUE(set_swap(6000000, 5000, kDest, "12345"));
  Tx t = base_tx();
  t.msgs[0] = send_msg(coin("6000000", "uatom") + "," + coin("1", "uosmo"), kDest);
  EXPECT_NE(run_swap(t.json(), GetParam()), parser_ok);
}

TEST_P(SwapGate, SecondCoinInFeeIsRefused) {
  ASSERT_TRUE(set_swap(6000000, 5000, kDest, "12345"));
  Tx t = base_tx();
  t.fee_coins = coin("5000", "uatom") + "," + coin("99", "uosmo");
  EXPECT_NE(run_swap(t.json(), GetParam()), parser_ok);
}

TEST_P(SwapGate, OtherDenomIsRefused) {
  ASSERT_TRUE(set_swap(6000000, 5000, kDest, "12345"));
  for (const char *denom : {"ATOM", "uosmo", "uatomx", "Uatom"}) {
    Tx t = base_tx();
    t.msgs[0] = send_msg(coin("6000000", denom), kDest);
    EXPECT_NE(run_swap(t.json(), GetParam()), parser_ok) << denom;
  }
}

TEST_P(SwapGate, OtherAmountEncodingsAreRefused) {
  ASSERT_TRUE(set_swap(6000000, 5000, kDest, "12345"));
  for (const char *amt : {"06000000", "6000000.0", "6000001", "600000"}) {
    Tx t = base_tx();
    t.msgs[0] = send_msg(coin(amt, "uatom"), kDest);
    EXPECT_NE(run_swap(t.json(), GetParam()), parser_ok) << amt;
  }
}

TEST_P(SwapGate, OtherRecipientIsRefused) {
  ASSERT_TRUE(set_swap(6000000, 5000, kDest, "12345"));
  Tx t = base_tx();
  t.msgs[0] = send_msg(coin("6000000", "uatom"),
                       std::string(kDest) + "q");  // longer, same prefix
  EXPECT_NE(run_swap(t.json(), GetParam()), parser_ok);
  t.msgs[0] = send_msg(coin("6000000", "uatom"), kFrom);
  EXPECT_NE(run_swap(t.json(), GetParam()), parser_ok);
}

TEST_P(SwapGate, MemoVariantsAreRefused) {
  ASSERT_TRUE(set_swap(6000000, 5000, kDest, "12345"));
  EXPECT_NE(run_swap(base_tx("123456").json(), GetParam()), parser_ok);
  EXPECT_NE(run_swap(base_tx("").json(), GetParam()), parser_ok);
  ASSERT_TRUE(set_swap(6000000, 5000, kDest, ""));
  EXPECT_NE(run_swap(base_tx("12345").json(), GetParam()), parser_ok);
}

// ---------------------------------------------------------------------------
// Transaction shapes: extra messages, MsgMultiSend, MsgExec, extra roots.
// ---------------------------------------------------------------------------
TEST_P(SwapGate, SecondMessageIsRefused) {
  ASSERT_TRUE(set_swap(6000000, 5000, kDest, "12345"));
  Tx t = base_tx();
  t.msgs.push_back(send_msg(coin("1", "uatom"), kDest));
  EXPECT_NE(run_swap(t.json(), GetParam()), parser_ok);
  // Same, without memo, so the extra items cannot be traded against Memo.
  ASSERT_TRUE(set_swap(6000000, 5000, kDest, ""));
  Tx u = base_tx("");
  u.msgs.push_back(send_msg(coin("1", "uatom"), kDest));
  EXPECT_NE(run_swap(u.json(), GetParam()), parser_ok);
}

TEST_P(SwapGate, MultiSendIsRefused) {
  ASSERT_TRUE(set_swap(6000000, 5000, kDest, "12345"));
  Tx t = base_tx();
  t.msgs[0] = std::string(R"({"type":"cosmos-sdk/MsgMultiSend","value":{"inputs":[{"address":")") +
              kFrom + R"(","coins":[)" + coin("6000000", "uatom") +
              R"(]}],"outputs":[{"address":")" + kDest + R"(","coins":[)" +
              coin("6000000", "uatom") + "]}]}}";
  EXPECT_NE(run_swap(t.json(), GetParam()), parser_ok);
}

TEST_P(SwapGate, ExecWrappingSendIsRefused) {
  ASSERT_TRUE(set_swap(6000000, 5000, kDest, "12345"));
  Tx t = base_tx();
  t.msgs[0] = std::string(R"({"type":"cosmos-sdk/MsgExec","value":{"grantee":")") +
              kFrom + R"(","msgs":[)" + send_msg(coin("6000000", "uatom"), kDest) +
              "]}}";
  EXPECT_NE(run_swap(t.json(), GetParam()), parser_ok);
}

TEST_P(SwapGate, TimeoutHeightOrTipChangesTheItemCount) {
  ASSERT_TRUE(set_swap(6000000, 5000, kDest, "12345"));
  Tx t = base_tx();
  t.after_sequence = R"(,"timeout_height":"10")";
  EXPECT_NE(run_swap(t.json(), GetParam()), parser_ok);
}

TEST_P(SwapGate, UnlistedChainIsRefused) {
  ASSERT_TRUE(set_swap(6000000, 5000, kDest, "12345"));
  Tx t = base_tx();
  t.chain = "theta-testnet-001";
  EXPECT_EQ(run_swap(t.json(), GetParam()), parser_swap_wrong_chain_id);
}

// Fee payer / granter are hidden outside expert mode on the default chain and
// do not enter the item count there. A payer or a granter only moves who pays
// the (unchanged, validated) fee.
TEST(SwapGateFee, GranterIsAcceptedOnlyInNormalModeOnDefaultChain) {
  ASSERT_TRUE(set_swap(6000000, 5000, kDest, "12345"));
  Tx t = base_tx();
  t.fee_extra = R"(,"granter":"cosmos1granter")";
  EXPECT_EQ(run_swap(t.json(), false), parser_ok);
  EXPECT_NE(run_swap(t.json(), true), parser_ok);
  app_mode_set_expert(0);
}

INSTANTIATE_TEST_SUITE_P(NormalAndExpert, SwapGate, ::testing::Values(false, true));

// ---------------------------------------------------------------------------
// The transaction has to be on the chain Exchange approved and priced the swap
// in, not on any listed chain whose amount, fee and recipient render the same.
// ---------------------------------------------------------------------------
TEST(SwapGateChain, TransactionOnAnotherListedChainIsRefused) {
  ASSERT_TRUE(set_swap(6000000, 5000, kDest, "12345", "osmo"));
  EXPECT_EQ(run_swap(base_tx().json(), false), parser_swap_wrong_chain_id);
  EXPECT_EQ(run_swap(base_tx().json(), true), parser_swap_wrong_chain_id);
  app_mode_set_expert(0);
}

TEST(SwapGateChain, UnknownOrMissingCoinConfigurationIsRefused) {
  EXPECT_FALSE(set_swap(6000000, 5000, kDest, "12345", "inj"));
  EXPECT_FALSE(set_swap(6000000, 5000, kDest, "12345", ""));
  EXPECT_FALSE(set_swap(6000000, 5000, kDest, "12345", std::string("cosmos\0", 7)));
}

// ---------------------------------------------------------------------------
// check_address only vouches for addresses on paths the app signs from.
// ---------------------------------------------------------------------------
namespace {
int check_address(const std::vector<uint32_t> &path, const std::string &hrp,
                  bool use_matching_address = true) {
  std::vector<uint8_t> cfg = {(uint8_t)hrp.size()};
  cfg.insert(cfg.end(), hrp.begin(), hrp.end());
  std::vector<uint8_t> ap = {(uint8_t)path.size()};
  for (uint32_t v : path) {
    ap.push_back(v >> 24);
    ap.push_back(v >> 16);
    ap.push_back(v >> 8);
    ap.push_back(v);
  }
  char expected[100];
  snprintf(expected, sizeof(expected), "%s1%08x%08x%08x", hrp.c_str(), path[2],
           path[3], path[4]);
  std::string addr = use_matching_address ? expected : "cosmos1attacker";
  check_address_parameters_t p;
  memset(&p, 0, sizeof(p));
  p.coin_configuration = cfg.data();
  p.coin_configuration_length = (uint8_t)cfg.size();
  p.address_parameters = ap.data();
  p.address_parameters_length = (uint8_t)ap.size();
  p.address_to_check = (char *)addr.c_str();
  handle_check_address(&p);
  return p.result;
}
const uint32_t H = 0x80000000u;
} // namespace

TEST(SwapCheckAddress, StandardPathIsAccepted) {
  EXPECT_EQ(check_address({H | 44, H | 118, H | 0, 0, 0}, "cosmos"), 1);
  EXPECT_EQ(check_address({H | 44, H | 118, H | 0, 0, 0}, "cosmos", false), 0);
}

TEST(SwapCheckAddress, PathsSigningRefusesAreRefused) {
  // change != 0 and purpose != 44' are refused by signing in every mode.
  EXPECT_EQ(check_address({H | 44, H | 118, H | 0, 1, 0}, "cosmos"), 0);
  EXPECT_EQ(check_address({H | 44, H | 118, H | 0, H | 7, 0}, "osmo"), 0);
  EXPECT_EQ(check_address({H | 49, H | 118, H | 0, 0, 0}, "cosmos"), 0);
  // Indices above 100 need expert mode to sign; swaps hold to normal mode.
  for (const bool expert : {false, true}) {
    app_mode_set_expert(expert ? 1 : 0);
    EXPECT_EQ(check_address({H | 44, H | 118, H | 0x7FFFFFFF, 0, 0x7FFFFFFF},
                            "cosmos"),
              0);
    EXPECT_EQ(check_address({H | 44, H | 118, H | 0, 0, 101}, "cosmos"), 0);
  }
  app_mode_set_expert(0);
  EXPECT_EQ(check_address({H | 44, H | 118, H | 0, 0, 100}, "cosmos"), 1);
}

TEST(SwapCheckAddress, OtherCoinTypeForListedHrpIsRefused) {
  EXPECT_EQ(check_address({H | 44, H | 60, H | 0, 0, 0}, "cosmos"), 0);
  EXPECT_EQ(check_address({H | 44, H | 60, H | 0, 0, 0}, "inj"), 0);
  EXPECT_EQ(check_address({H | 44, H | 118, H | 0, 0, 0}, "neutron"), 0);
}

// ---------------------------------------------------------------------------
// get_printable_amount and the signing comparison format the same base units.
// ---------------------------------------------------------------------------
namespace {
std::string printable(uint64_t amount, const std::string &cfg_s) {
  std::vector<uint8_t> a = be(amount);
  std::vector<uint8_t> cfg = {(uint8_t)cfg_s.size()};
  cfg.insert(cfg.end(), cfg_s.begin(), cfg_s.end());
  get_printable_amount_parameters_t p;
  memset(&p, 0, sizeof(p));
  p.coin_configuration = cfg.data();
  p.coin_configuration_length = (uint8_t)cfg.size();
  p.amount = a.data();
  p.amount_length = (uint8_t)a.size();
  handle_get_printable_amount(&p);
  return p.printable_amount;
}
} // namespace

TEST(SwapPrintable, Formats) {
  EXPECT_EQ(printable(6000000, "cosmos"), "6.000000 ATOM");
  EXPECT_EQ(printable(1, "osmo"), "0.000001 OSMO");
  EXPECT_EQ(printable(1000000000000000000ULL, "dydx"),
            "1.000000000000000000 DYDX");
  EXPECT_EQ(printable(0, "cosmos"), "0.000000 ATOM");
  EXPECT_EQ(printable(5, "inj"), "");
}

// An HRP or coin configuration with an embedded NUL must not resolve to a
// listed chain.
TEST(SwapCheckAddress, EmbeddedNulInHrpIsRefused) {
  for (const std::string hrp :
       {std::string("cosmos\0", 7), std::string("cosm\0s", 6),
        std::string("osmo\0X", 6), std::string("\0cosmos", 7)}) {
    EXPECT_EQ(check_address({H | 44, H | 118, H | 0, 0, 0}, hrp), 0);
    EXPECT_EQ(printable(6000000, hrp), "");
  }
}
