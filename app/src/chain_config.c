/*******************************************************************************
 *   (c) 2018 - 2023 Zondax AG
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
#include "chain_config.h"

#include <zxmacros.h>

typedef struct {
  const uint32_t path;
  const char *hrp;
  const address_encoding_e encoding;
} chain_config_t;

// To enable custom config for a new chain, just add a new entry in this array
// with path, hrp and encoding
static const chain_config_t chainConfig[] = {
    {118, "cosmos", BECH32_COSMOS},   {60, "inj", BECH32_ETH},
    {60, "evmos", BECH32_ETH},        {60, "xpla", BECH32_ETH},
    {60, "dym", BECH32_ETH},          {60, "zeta", BECH32_ETH},
    {60, "bera", BECH32_ETH},         {60, "human", BECH32_ETH},
    {118, "osmo", BECH32_COSMOS},     {118, "dydx", BECH32_COSMOS},
    {118, "mantra", BECH32_COSMOS},   {118, "xion", BECH32_COSMOS},
    {118, "celestia", BECH32_COSMOS}, {118, "core", BECH32_COSMOS},
    {118, "neutron", BECH32_COSMOS}};

static const uint32_t chainConfigLen =
    sizeof(chainConfig) / sizeof(chainConfig[0]);

address_encoding_e checkChainConfig(uint32_t path, const char *hrp,
                                    uint8_t hrpLen) {
  if (hrp == NULL || hrpLen == 0) {
    return UNSUPPORTED;
  }

  // Reject anything that is not a well-formed bech32 HRP before the table is
  // consulted. The load-bearing case is the embedded NUL: the table match below
  // compares hrpLen against strlen() of the entry, so a declared "inj\0X"
  // (hrpLen 5) misses the "inj" entry, falls through to the generic 118'
  // fallback, and is then handed to bech32EncodeFromBytes(), which measures the
  // HRP with strlen() and truncates it back to "inj" -- reintroducing exactly
  // the Injective address derived on the Cosmos 118' path that the table-first
  // ordering exists to refuse. Non-printable, non-ASCII and uppercase bytes are
  // rejected here too: bech32_encode() already refuses them, but only after the
  // chain has been declared supported, so catching them at the single decision
  // point keeps the status word accurate.
  for (uint8_t i = 0; i < hrpLen; i++) {
    const uint8_t ch = (uint8_t)hrp[i];
    if (ch < 33 || ch > 126 || (ch >= 'A' && ch <= 'Z')) {
      return UNSUPPORTED;
    }
  }

  // Resolve the HRP against the table before anything else. An entry pins both
  // the coin type and the address algorithm for that chain, so a request naming
  // a known HRP on some other path has to be refused: taking the default Cosmos
  // branch first meant "inj" on the generic 118' path derived a secp256k1
  // Cosmos address and handed it back as if it were an Injective one, even
  // though Injective needs the Ethereum-style 60' derivation.
  for (uint32_t i = 0; i < chainConfigLen; i++) {
    const char *hrpPtr = (const char *)PIC(chainConfig[i].hrp);
    const uint16_t hrpPtrLen = strlen(hrpPtr);
    if (hrpPtrLen == hrpLen && memcmp(hrpPtr, hrp, hrpLen) == 0) {
      if (path == (0x80000000u | chainConfig[i].path)) {
        return chainConfig[i].encoding;
      }
      return UNSUPPORTED;
    }
  }

  // Unrecognised HRP: the generic Cosmos path stays open, which is what keeps
  // the long tail of 118' chains that have no entry here working.
  if (path == HDPATH_1_DEFAULT) {
    return BECH32_COSMOS;
  }

  return UNSUPPORTED;
}
