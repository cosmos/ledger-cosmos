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
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "coin.h"
#include <stdbool.h>
#include <stdint.h>

// Longest HRP accepted for an account address. Real account prefixes are a
// dozen characters at most; a longer one leaves room to embed a whole foreign
// address at the start of the one the device shows.
#define MAX_ACCOUNT_HRP_LEN 20u

address_encoding_e checkChainConfig(uint32_t path, const char *hrp,
                                    uint8_t hrpLen);

// `path` is the hardened coin type element of the derivation path (hdPath[1]),
// as taken by checkChainConfig.
bool isSupportedCoinType(uint32_t path);

typedef enum {
  HD_PATH_OK,
  HD_PATH_INVALID,      // not 44'/<supported coin>'/x/0/y
  HD_PATH_NEEDS_EXPERT, // an account, change or index element above 100
} hd_path_check_e;

// The derivation path policy every flow that derives a key applies, so that an
// address the app hands out (to the host or to Exchange) is always one it will
// also sign from.
hd_path_check_e checkHDPath(const uint32_t path[HDPATH_LEN_DEFAULT],
                            bool expert);

#ifdef __cplusplus
}
#endif
