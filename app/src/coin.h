/*******************************************************************************
*  (c) 2019-2021 Zondax GmbH
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

#define CLA                  0x55u

#define HDPATH_LEN_DEFAULT   5

#define HDPATH_0_DEFAULT     (0x80000000u | 0x2cu)
#define HDPATH_1_DEFAULT     (0x80000000u | 0x76u)
#define HDPATH_ETH_1_DEFAULT (0x80000000u | 0x3cu)
#define HDPATH_2_DEFAULT     (0x80000000u | 0u)
#define HDPATH_3_DEFAULT     (0u)

#define PK_LEN_SECP256K1     33u
#define PK_LEN_SECP256K1_UNCOMPRESSED   65u

typedef enum {
    addr_secp256k1 = 0,
} address_kind_e;

typedef enum {
    tx_json = 0,
    tx_textual
} tx_type_e;

typedef enum {
    BECH32_COSMOS = 0,
    BECH32_ETH,
    UNSUPPORTED = 0xFF,
} address_encoding_e;

#define VIEW_ADDRESS_OFFSET_SECP256K1       PK_LEN_SECP256K1
#define VIEW_ADDRESS_LAST_PAGE_DEFAULT      0

#define MENU_MAIN_APP_LINE1                "Cosmos"
#define MENU_MAIN_APP_LINE2                "Ready"
#define APPVERSION_LINE1                   "Version:"
#define APPVERSION_LINE2                   ("v" APPVERSION)

#define COIN_DEFAULT_CHAINID                "cosmoshub-4"
#define OSMOSIS_CHAINID                     "osmosis-1"
#define DYDX_CHAINID                        "dydx-mainnet-1"
#define MANTRA_CHAINID                      "mantra-1"
#define XION_CHAINID                        "xion-mainnet-1"
#define CELESTIA_CHAINID                    "celestia"

// In non-expert mode, the app will convert from uatom to ATOM
#define COIN_DEFAULT_DENOM_BASE             "uatom"
#define COIN_DEFAULT_DENOM_REPR             "ATOM"
#define COIN_DEFAULT_DENOM_FACTOR           6u
#define COIN_DEFAULT_DENOM_TRIMMING         6u

// Coin denoms may be up to 128 characters long
// https://github.com/cosmos/cosmos-sdk/blob/master/types/coin.go#L780
// https://github.com/cosmos/ibc-go/blob/main/docs/architecture/adr-001-coin-source-tracing.md
#define COIN_DENOM_MAXSIZE                  129
#define COIN_AMOUNT_MAXSIZE                 50

#define COIN_MAX_CHAINID_LEN                20u
#define INDEXING_TMP_KEYSIZE                70u
#define INDEXING_TMP_VALUESIZE              70u
#define INDEXING_GROUPING_REF_TYPE_SIZE     70u
#define INDEXING_GROUPING_REF_FROM_SIZE     70u

#define MENU_MAIN_APP_LINE2_SECRET         "?"
#define COIN_SECRET_REQUIRED_CLICKS         0

#define INS_GET_VERSION                 0x00
#define INS_SIGN_SECP256K1              0x02u
#define INS_GET_ADDR_SECP256K1          0x04u


// Custom errors
#define APDU_CODE_TRANSACTION_DATA_EXCEEDS_BUFFER_CAPACITY 0x6988
#define APDU_CODE_INVALID_HD_PATH_VALUE 0x6989
#define APDU_CODE_HRP_WRONG_LENGTH 0x698A
#define APDU_CODE_INVALID_HD_PATH_COIN_VALUE 0x698B
#define APDU_CODE_CHAIN_CONFIG_NOT_SUPPORTED 0x698C
#define APDU_CODE_EXPERT_MODE_REQUIRED_FOR_ETH_CHAIN 0x698D


#ifdef __cplusplus
}
#endif
