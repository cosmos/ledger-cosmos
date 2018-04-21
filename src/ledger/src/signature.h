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

// Creates signature of the message using (...TODO)
// @return
// *   1 if signature is verified
// *   0 is signature is not verified
int generate_signature_SECP256K1(
    uint8_t *message,
    uint16_t message_length);

// Creates signature of the message using (...TODO)
// @return
// *   1 if signature is verified
// *   0 is signature is not verified
int generate_signature_ED25519(
    uint8_t *message,
    uint16_t message_length);

void generate_public_key_from_bip();

// Returns last created signature
uint8_t* get_response_buffer();

// Returns last created signature's length
unsigned int get_response_buffer_length();