/*******************************************************************************
 *   (c) 2026 Zondax AG
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

// Host build of app/src/swap: the device-only symbols the swap handlers call.
#ifdef __cplusplus
extern "C" {
#endif

#include "coin.h"
#include "zxerror.h"
#include <stdint.h>

void os_explicit_zero_BSS_segment(void);
void os_lib_end(void);

zxerr_t crypto_swap_fillAddress(uint32_t *hdPath_swap, uint8_t hdPathLen_swap,
                                char *hrp, address_encoding_e encode_type,
                                char *buffer, uint16_t bufferLen,
                                uint16_t *addrResponseLen);

#ifdef __cplusplus
}
#endif
