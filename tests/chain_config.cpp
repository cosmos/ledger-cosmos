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

#include "chain_config.h"
#include <cstring>

// checkChainConfig() is the single decision point behind both address paths:
// GET_ADDR_SECP256K1 (apdu_handler.c) turns UNSUPPORTED into
// APDU_CODE_CHAIN_CONFIG_NOT_SUPPORTED, and handle_check_address() (swap)
// turns it into "no match", leaving params->result at 0. Everything asserted
// here therefore holds on both.
namespace {

address_encoding_e check(uint32_t coinType, const char *hrp) {
  return checkChainConfig(0x80000000u | coinType, hrp,
                          static_cast<uint8_t>(strlen(hrp)));
}

}  // namespace

// An entry in the table pins the coin type *and* the address algorithm for
// that chain. "inj" means the Ethereum-style 60' derivation; asking for it on
// the generic Cosmos 118' path used to take the "always allowed for 118"
// branch first and hand back a secp256k1 Cosmos address labelled inj1... --
// an address the user cannot receive on, from a request the device accepted.
TEST(ChainConfig, KnownHrpOnTheGenericCosmosPathIsRefused) {
  EXPECT_EQ(check(118, "inj"), UNSUPPORTED);
}

TEST(ChainConfig, EveryEthHrpIsRefusedOnTheCosmosPath) {
  for (const char *hrp : {"inj", "evmos", "xpla", "dym", "zeta", "bera", "human"}) {
    EXPECT_EQ(check(118, hrp), UNSUPPORTED) << hrp << " was accepted on 118'";
  }
}

// The mirror case, and the one that was already covered at zemu level: a
// Cosmos HRP on the Ethereum path.
TEST(ChainConfig, KnownHrpOnTheWrongEthPathIsRefused) {
  for (const char *hrp : {"cosmos", "osmo", "dydx", "celestia"}) {
    EXPECT_EQ(check(60, hrp), UNSUPPORTED) << hrp << " was accepted on 60'";
  }
}

// A table hit on its own path keeps its own encoding.
TEST(ChainConfig, EthChainsResolveOnTheirOwnPath) {
  for (const char *hrp : {"inj", "evmos", "xpla", "dym", "zeta", "bera", "human"}) {
    EXPECT_EQ(check(60, hrp), BECH32_ETH) << hrp;
  }
}

TEST(ChainConfig, CosmosChainsResolveOnTheirOwnPath) {
  for (const char *hrp :
       {"cosmos", "osmo", "dydx", "mantra", "xion", "celestia", "core", "neutron"}) {
    EXPECT_EQ(check(118, hrp), BECH32_COSMOS) << hrp;
  }
}

// What the reordering must not break: the long tail of 118' chains that have
// no entry here at all still has to work, or the app stops signing for most of
// the ecosystem.
TEST(ChainConfig, UnknownHrpKeepsTheGenericCosmosPath) {
  for (const char *hrp : {"akash", "juno", "regen", "stars", "kava"}) {
    EXPECT_EQ(check(118, hrp), BECH32_COSMOS) << hrp;
  }
}

TEST(ChainConfig, UnknownHrpOnAnyOtherPathIsRefused) {
  for (const uint32_t coinType : {0u, 1u, 60u, 119u, 529u}) {
    EXPECT_EQ(check(coinType, "akash"), UNSUPPORTED) << coinType;
  }
}

// Matching is length-exact, so a longer HRP that starts with a table entry is
// not that entry -- "cosmoshub" is an unknown chain, not "cosmos".
TEST(ChainConfig, HrpMatchIsLengthExact) {
  EXPECT_EQ(check(118, "cosmoshub"), BECH32_COSMOS);  // unknown -> generic path
  EXPECT_EQ(check(60, "cosmoshub"), UNSUPPORTED);
  EXPECT_EQ(check(60, "injective"), UNSUPPORTED);     // not the "inj" entry
  EXPECT_EQ(check(118, "injective"), BECH32_COSMOS);  // unknown -> generic path
}

// The caller passes the length separately from the pointer; a prefix of a
// known HRP has to resolve as that prefix and nothing else.
TEST(ChainConfig, HrpLengthBoundsTheComparison) {
  EXPECT_EQ(checkChainConfig(0x80000000u | 60u, "injective", 3), BECH32_ETH);
  EXPECT_EQ(checkChainConfig(0x80000000u | 118u, "injective", 3), UNSUPPORTED);
}

// A non-hardened coin type is never a table hit: the entries are compared
// against `0x80000000 | path`.
TEST(ChainConfig, NonHardenedPathIsRefused) {
  EXPECT_EQ(checkChainConfig(118, "cosmos", 6), UNSUPPORTED);
  EXPECT_EQ(checkChainConfig(60, "inj", 3), UNSUPPORTED);
  EXPECT_EQ(checkChainConfig(118, "akash", 5), UNSUPPORTED);
}
