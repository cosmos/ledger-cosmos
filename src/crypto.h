/*******************************************************************************
*   (c) 2016 Ledger
*   (c) 2018 ZondaX GmbH
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

#include <stdint.h>
#include "os.h"
#include "cx.h"

#define MAX_BECH32_HRP_LEN      83
#define PK_COMPRESSED_LEN       33

#define BIP32_LEN_DEFAULT   5
#define BIP32_0_DEFAULT     (0x80000000 | 0x2c)
#define BIP32_1_DEFAULT     (0x80000000 | 0x76)
#define BIP32_2_DEFAULT     (0x80000000 | 0)
#define BIP32_3_DEFAULT     (0)
#define BIP32_4_DEFAULT     (0)

#define BIP32_NO_ERROR              0
#define BIP32_INVALID_LENGTH        -1
#define BIP32_INVALID_PATH          -2

extern char bech32_hrp[MAX_BECH32_HRP_LEN + 1];
extern uint8_t bech32_hrp_len;
extern cx_ecfp_public_key_t publicKey;

void crypto_init();

int32_t getBip32Account();
int32_t getBip32Index();
void setBip32Index(uint32_t newIndex);
int8_t setBip32Path(uint32_t path0,
                    uint32_t path1,
                    uint32_t path2,
                    uint32_t path3,
                    uint32_t path4);

void updatePubKey();
void getPubKeyCompressed(uint8_t *pkc);
void getBech32Addr(char *bech32_addr);

int sign_secp256k1(const uint8_t *message,
                   unsigned int message_length,
                   uint8_t *signature,
                   unsigned int signature_capacity,
                   unsigned int *signature_length);
