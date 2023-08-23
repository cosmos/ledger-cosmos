/*******************************************************************************
*   (c) 2021 Zondax GmbH
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
#include <parser.h>
#include <sstream>
#include <string>
#include "common.h"
#include <tx_display.h>
#include <fmt/core.h>
#include "keccak.h"
#include "bech32.h"
#include "hexutils.h"
#include "gmock/gmock.h"
#include "coin.h"

std::vector<std::string> dumpUI(parser_context_t *ctx,
                                uint16_t maxKeyLen,
                                uint16_t maxValueLen) {
    auto answer = std::vector<std::string>();

    uint8_t numItems;
    parser_error_t err = parser_getNumItems(ctx, &numItems);
    if (err != parser_ok) {
        return answer;
    }

    for (uint16_t idx = 0; idx < numItems; idx++) {
        char keyBuffer[1000];
        char valueBuffer[1000];
        uint8_t pageIdx = 0;
        uint8_t pageCount = 1;

        while (pageIdx < pageCount) {
            std::stringstream ss;

            err = parser_getItem(ctx,
                                 idx,
                                 keyBuffer, maxKeyLen,
                                 valueBuffer, maxValueLen,
                                 pageIdx, &pageCount);

            ss << fmt::format("{} | {}", idx, keyBuffer);
            if (pageCount > 1) {
                ss << fmt::format(" [{}/{}]", pageIdx + 1, pageCount);
            }
            ss << " : ";

            if (err == parser_ok) {
                // Model multiple lines
                ss << fmt::format("{}", valueBuffer);
            } else {
                ss << parser_getErrorDescription(err);
            }

            auto output = ss.str();
            if (output.back() == ' ') {
                output = output.substr(0, output.size() - 1);
            }

            answer.push_back(output);

            pageIdx++;
        }
    }

    return answer;
}

TEST(Address, EVMAddressEvmos) {
    // const char compressedPubkey = "027d8d3c470d1cfd8525d9537efdb92319a13a9bc9e336b6621fa5a664d2591b60";
    const char uncompressedPubkey[] = "047d8d3c470d1cfd8525d9537efdb92319a13a9bc9e336b6621fa5a664d2591b60fcd4f7882b0ff07d5ea0697050c7d23428daa5beaf6268cbac1369c278c6d8ea";

    uint8_t pubkey[PK_LEN_SECP256K1_UNCOMPRESSED] = {0};
    parseHexString(pubkey, sizeof(pubkey), uncompressedPubkey);

    uint8_t hash[32] = {0};
    keccak_hash(pubkey+1, sizeof(pubkey)-1, hash, sizeof(hash));    // Skip prefix 0x04
    const uint8_t *eth_address = &hash[12];

    // Keccak256(pubkey)
    uint8_t expectedHash[32] = {0};
    parseHexString(expectedHash, sizeof(expectedHash), "6b5eab3a8cb5e79f21f0cc6b6cbcd73cd8e8a42844662f0a0e76d7f79afd933d");
    for (int i = 0; i < 32; i++) {
        ASSERT_EQ(hash[i], expectedHash[i]);
    }

    const char bech32_hrp[] = "evmos";
    char address[100] = {0};
    const zxerr_t err = bech32EncodeFromBytes(address, sizeof(address), bech32_hrp, eth_address, 20, 0, BECH32_ENCODING_BECH32);

    const std::string evm_address(address, address + strnlen(address, sizeof(address)));
    EXPECT_EQ(evm_address, "evmos1dj7dw0xcazjzs3rx9u9quakh77d0myeamrkupf");
}

TEST(Address, EVMAddressCosmos) {
    // const char compressedPubkey = "022374f2dacd71042b5a888e3839e4ba54752ad6a51d35b54f6abb899c4329d4bf";
    const char uncompressedPubkey[] = "042374f2dacd71042b5a888e3839e4ba54752ad6a51d35b54f6abb899c4329d4bfb455e3086720cb6543212ca4c8a3cce80bb3938ec90baaabfc90930f2b9ac660";

    uint8_t pubkey[PK_LEN_SECP256K1_UNCOMPRESSED] = {0};
    parseHexString(pubkey, sizeof(pubkey), uncompressedPubkey);

    uint8_t hash[32] = {0};
    keccak_hash(pubkey+1, sizeof(pubkey)-1, hash, sizeof(hash));    // Skip prefix 0x04
    const uint8_t *eth_address = &hash[12];

    // Keccak256(pubkey)
    uint8_t expectedHash[32] = {0};
    parseHexString(expectedHash, sizeof(expectedHash), "c21055c2f5317e96f73298e4a4d577fc4c4a3073553bd25a4e9cb3f1cdace549");
    for (int i = 0; i < 32; i++) {
        ASSERT_EQ(hash[i], expectedHash[i]);
    }

    const char bech32_hrp[] = "cosmos";
    char address[100] = {0};
    const zxerr_t err = bech32EncodeFromBytes(address, sizeof(address), bech32_hrp, eth_address, 20, 0, BECH32_ENCODING_BECH32);

    const std::string evm_address(address, address + strnlen(address, sizeof(address)));
    EXPECT_EQ(evm_address, "cosmos15n2h0lzvfgc8x4fm6fdya89n78x6ee2fm7fxr3");
}
