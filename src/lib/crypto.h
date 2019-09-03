/*******************************************************************************
*   (c) 2019 ZondaX GmbH
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

#include <zxmacros.h>
#include "cosmos.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_BECH32_HRP_LEN      83
#define PK_COMPRESSED_LEN       33

extern uint32_t bip32Path[BIP32_LEN_DEFAULT];
extern char *hrp;

#define PUBKEY_LEN 33

uint8_t extractHRP(uint32_t rx, uint32_t offset);

void crypto_set_hrp(char *p);

uint16_t crypto_fillAddress(uint8_t *buffer, uint16_t buffer_len);

uint16_t crypto_sign(uint8_t *signature, uint16_t signatureMaxlen,
                     const uint8_t *message, uint16_t messageLen);

#ifdef __cplusplus
}
#endif
