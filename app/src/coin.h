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

#define HDPATH_0_DEFAULT     (0x80000000 | 0x2c)
#define HDPATH_1_DEFAULT     (0x80000000 | 0x76)
#define HDPATH_2_DEFAULT     (0x80000000 | 0)
#define HDPATH_3_DEFAULT     (0)
#define HDPATH_4_DEFAULT     (0)

typedef enum {
    addr_secp256k1 = 0,
} address_kind_e;

#define MENU_MAIN_APP_LINE1 "Cosmos"
#ifdef TESTING_ENABLED
#define MENU_MAIN_APP_LINE2 "Cosmos TEST!"
#else
#define MENU_MAIN_APP_LINE2 "App"
#endif
#define APPVERSION_LINE2 ""

#define VIEW_ADDRESS_ITEM_COUNT 2
#define VIEW_ADDRESS_BUFFER_OFFSET    (PK_LEN)

#ifdef __cplusplus
}
#endif
