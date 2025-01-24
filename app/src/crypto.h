/*******************************************************************************
*   (c) 2019 Zondax GmbH
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

#include <zxmacros.h>
#include "coin.h"
#include <stdbool.h>
#include <sigutils.h>
#include "zxerror.h"

#define MAX_BECH32_HRP_LEN      83u

extern uint32_t hdPath[HDPATH_LEN_DEFAULT];
extern char bech32_hrp[MAX_BECH32_HRP_LEN + 1];
extern uint8_t bech32_hrp_len;
extern address_encoding_e encoding;

zxerr_t crypto_fillAddress(uint8_t *buffer, uint16_t bufferLen, uint16_t *addrResponseLen);

zxerr_t crypto_sign(uint8_t *signature, uint16_t signatureMaxlen, uint16_t *signatureLen);

zxerr_t crypto_swap_fillAddress(uint32_t *hdPath_swap,
                                      uint8_t hdPathLen_swap,
                                      char *hrp,
                                      address_encoding_e encode_type,
                                      char *buffer,
                                      uint16_t bufferLen,
                                      uint16_t *addrResponseLen);
#ifdef __cplusplus
}
#endif
