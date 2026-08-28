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

#include "bech32.h"
#include "chain_config.h"
#include <cstring>
#include <string>

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

// ---------------------------------------------------------------------------
// HRP well-formedness
//
// The caller declares the HRP length out of band (an APDU byte, or the swap
// coin_configuration), so the bytes in between are attacker-chosen and are not
// necessarily a C string. Everything downstream of this function measures the
// HRP with strlen(): bech32EncodeFromBytes() does, and so does the table match
// below. That mismatch is the whole problem, so the bytes are validated here,
// at the single decision point, before either consumer sees them.
// ---------------------------------------------------------------------------

// The load-bearing case. "inj\0X" declared as 5 bytes is not the "inj" table
// entry (strlen("inj") != 5), so before this check it fell through to the
// generic 118' fallback and was accepted -- and the encoder then truncated it
// straight back to "inj". Net effect: an inj1... address derived on the Cosmos
// 118' path with secp256k1/ripemd160 instead of Injective's keccak/60' path,
// which is exactly what KnownHrpOnTheGenericCosmosPathIsRefused exists to stop.
TEST(ChainConfig, EmbeddedNulCannotSmuggleAKnownHrpOntoTheCosmosPath) {
  EXPECT_EQ(checkChainConfig(0x80000000u | 118u, "inj\0X", 5), UNSUPPORTED);
  EXPECT_EQ(checkChainConfig(0x80000000u | 60u, "cosmos\0X", 8), UNSUPPORTED);
}

// A trailing NUL inside the declared length is the same bypass with the same
// payload, and is refused for the same reason.
TEST(ChainConfig, TrailingNulInsideTheDeclaredLengthIsRefused) {
  EXPECT_EQ(checkChainConfig(0x80000000u | 118u, "cosmos\0", 7), UNSUPPORTED);
  EXPECT_EQ(checkChainConfig(0x80000000u | 118u, "akash\0", 6), UNSUPPORTED);
}

// Pins the downstream behaviour the check above compensates for. If zxlib ever
// grows a length-aware encoder this test is what says the gate can be revisited.
TEST(ChainConfig, Bech32EncoderMeasuresTheHrpWithStrlen) {
  const uint8_t payload[20] = {0};
  char truncated[128] = {0};
  char plain[128] = {0};

  ASSERT_EQ(bech32EncodeFromBytes(truncated, sizeof(truncated), "inj\0X", payload,
                                  sizeof(payload), 1, BECH32_ENCODING_BECH32),
            zxerr_ok);
  ASSERT_EQ(bech32EncodeFromBytes(plain, sizeof(plain), "inj", payload,
                                  sizeof(payload), 1, BECH32_ENCODING_BECH32),
            zxerr_ok);

  EXPECT_STREQ(truncated, plain);
}

// bech32 HRPs are lowercase. Uppercase was already refused by bech32_encode(),
// but only after the request had been declared supported -- so the device
// reported "invalid data" instead of "chain config not supported", and the
// rejection depended on a library two layers down.
TEST(ChainConfig, UppercaseHrpIsRefused) {
  EXPECT_EQ(checkChainConfig(0x80000000u | 118u, "INJ", 3), UNSUPPORTED);
  EXPECT_EQ(checkChainConfig(0x80000000u | 118u, "COSMOS", 6), UNSUPPORTED);
  EXPECT_EQ(checkChainConfig(0x80000000u | 118u, "Cosmos", 6), UNSUPPORTED);
  EXPECT_EQ(checkChainConfig(0x80000000u | 118u, "AKASH", 5), UNSUPPORTED);
  EXPECT_EQ(checkChainConfig(0x80000000u | 60u, "INJ", 3), UNSUPPORTED);
}

// bech32 restricts the HRP to printable ASCII, [33, 126]. Space and DEL sit
// just outside it on either side; 0x80 and 0xFF cover the high half.
TEST(ChainConfig, NonPrintableOrNonAsciiHrpIsRefused) {
  for (const char *hrp : {"in\x01j", "in j", "in\x7fj", "in\x80j", "in\xffj",
                          "in\tj", "in\nj"}) {
    EXPECT_EQ(checkChainConfig(0x80000000u | 118u, hrp, 4), UNSUPPORTED) << hrp;
  }
}

// The NUL does not have to sit after a table entry to be a problem: whatever
// the encoder finds before it is what the user is shown.
TEST(ChainConfig, LeadingNulIsRefused) {
  EXPECT_EQ(checkChainConfig(0x80000000u | 118u, "\0cosmos", 7), UNSUPPORTED);
  EXPECT_EQ(checkChainConfig(0x80000000u | 118u, "\0", 1), UNSUPPORTED);
}

// hrpLen is a uint8_t and so is the scan index. The callers cap the length at
// MAX_BECH32_HRP_LEN (83), but the function has to terminate on any value a
// uint8_t can hold, including 255.
TEST(ChainConfig, ScanTerminatesAtTheMaximumDeclaredLength) {
  const std::string longHrp(255, 'a');
  EXPECT_EQ(checkChainConfig(0x80000000u | 118u, longHrp.c_str(), 255),
            BECH32_COSMOS);

  std::string longHrpWithNul(255, 'a');
  longHrpWithNul[254] = '\0';
  EXPECT_EQ(checkChainConfig(0x80000000u | 118u, longHrpWithNul.c_str(), 255),
            UNSUPPORTED);
}

TEST(ChainConfig, NullOrEmptyHrpIsRefused) {
  EXPECT_EQ(checkChainConfig(0x80000000u | 118u, nullptr, 0), UNSUPPORTED);
  EXPECT_EQ(checkChainConfig(0x80000000u | 118u, nullptr, 6), UNSUPPORTED);
  EXPECT_EQ(checkChainConfig(0x80000000u | 118u, "cosmos", 0), UNSUPPORTED);
}

// The gate must not cost the long tail of valid HRPs anything: digits and the
// separators that appear in real chain prefixes are printable lowercase ASCII
// and still reach the generic 118' fallback.
TEST(ChainConfig, ValidPrintableLowercaseHrpsAreUnaffected) {
  for (const char *hrp : {"akash", "c4e", "0g", "e-money", "likecoin"}) {
    EXPECT_EQ(checkChainConfig(0x80000000u | 118u, hrp,
                               static_cast<uint8_t>(strlen(hrp))),
              BECH32_COSMOS)
        << hrp;
  }
}
