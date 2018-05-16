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

#include <stdbool.h>
#include "apdu_codes.h"
#include "common.h"

#define CLA                         0x55

#define OFFSET_CLA                  0
#define OFFSET_INS                  1  //< Instruction offset
#define OFFSET_PCK_INDEX            2  //< Package index offset
#define OFFSET_PCK_COUNT            3  //< Package count offset
#define OFFSET_DATA                 4  //< Data offset

#define INS_GET_VERSION                 0
#define INS_PUBLIC_KEY_SECP256K1        1
#define INS_PUBLIC_KEY_ED25519          2
#define INS_SIGN_SECP256K1              3
#define INS_SIGN_ED25519                4
#define INS_SIGN_SECP256K1_STDSIGNMSG   5
#define INS_SIGN_ED25519_STDSIGNMSG     6

#define INS_HASH_TEST                   100
#define INS_PUBLIC_KEY_SECP256K1_TEST   101
#define INS_PUBLIC_KEY_ED25519_TEST     102
#define INS_SIGN_SECP256K1_TEST         103
#define INS_SIGN_ED25519_TEST           104

void app_init();
void app_main();
bool extractBip32(uint8_t *depth, uint32_t path[10], uint32_t rx, uint32_t offset);
