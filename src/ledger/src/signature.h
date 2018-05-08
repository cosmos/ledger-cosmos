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
#include "os.h"

void keys_secp256k1(
        cx_ecfp_public_key_t* publicKey,
        cx_ecfp_private_key_t* privateKey,
        const uint8_t privateKeyData[32]);

int sign_secp256k1(
        const uint8_t* message,
        unsigned int message_length,
        uint8_t* signature,
        unsigned int signature_capacity,
        unsigned int* signature_length,
        cx_ecfp_private_key_t* privateKey);
