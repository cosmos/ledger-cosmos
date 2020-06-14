/*******************************************************************************
*  (c) 2019 Zondax GmbH
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

#include <stdint.h>
#include <stddef.h>

#define CLA                  0x55

#define HDPATH_LEN_DEFAULT   5

#define HDPATH_0_DEFAULT     (0x80000000u | 0x2cu)
#define HDPATH_1_DEFAULT     (0x80000000u | 0x76u)
#define HDPATH_2_DEFAULT     (0x80000000u | 0u)
#define HDPATH_3_DEFAULT     (0u)
#define HDPATH_4_DEFAULT     (0u)

#define PK_LEN_SECP256K1     33u

typedef enum {
    addr_secp256k1 = 0,
} address_kind_e;

#define VIEW_ADDRESS_OFFSET_SECP256K1       PK_LEN_SECP256K1
#define VIEW_ADDRESS_ITEM_COUNT             2
#define VIEW_ADDRESS_LAST_PAGE_DEFAULT      0

#define MENU_MAIN_APP_LINE1                "Cosmos"
#define MENU_MAIN_APP_LINE2                "ready"
#define APPVERSION_LINE1                   "Version:"
#define APPVERSION_LINE2                   ("v" APPVERSION)

#define CRYPTO_BLOB_SKIP_BYTES              0
#define COIN_DEFAULT_CHAINID                "cosmoshub-3"

// In non-expert mode, the app will convert from uatom to ATOM
#define COIN_DEFAULT_DENOM_BASE             "uatom"
#define COIN_DEFAULT_DENOM_REPR             "ATOM"
#define COIN_DEFAULT_DENOM_FACTOR           6

#ifdef __cplusplus
}
#endif
