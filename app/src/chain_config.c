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

// To enable custom config for a new chain, just add a new entry in this array with path, hrp and encoding
static const chain_config_t chainConfig[] = {
    // {118, cosmos, BECH32_COSMOS},
    {60, "inj", BECH32_ETH},
};

static const uint32_t chainConfigLen = sizeof(chainConfig) / sizeof(chainConfig[0]);

address_encoding_e checkChainConfig(uint32_t path, const char* hrp, uint8_t hrpLen) {
    // Always allowed for 118 (default Cosmos)
    if (path == HDPATH_1_DEFAULT) {
        return BECH32_COSMOS;
    }

    // Check special cases
    for (uint32_t i = 0; i < chainConfigLen; i++) {
        if (path == (0x80000000u | chainConfig[i].path)) {
            const char* hrpPtr = (const char *) PIC(chainConfig[i].hrp);
            const uint16_t hrpPtrLen = strlen(hrpPtr);
            if (hrpPtrLen == hrpLen && memcmp(hrpPtr, hrp, hrpLen) == 0) {
                return chainConfig[i].encoding;
            }
        }
    }

    return UNSUPPORTED;
}
