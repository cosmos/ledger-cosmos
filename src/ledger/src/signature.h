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

void bip32_private(
        const uint32_t* bip32_path,
        size_t bip32_len,
        uint8_t privateKeyData[32]);

void keys_secp256k1(
        cx_ecfp_public_key_t* publicKey,
        cx_ecfp_private_key_t* privateKey,
        const uint8_t privateKeyData[32]);

/**
 * Sign a message according to ECDSA specification.
 *
 * @param [in] message
 *   A raw message to be signed. Signature will be generated for the hash of the message.
 *
 * @param [in] message_length
 *   Length of the message.
 *
 * @param [in|out] signature
 *  A buffer where signature will be stored. Buffer must be created outside this function
 *  and need to have enough capacity to store the whole signature.
 *
 *  @param [in] signature_capacity
 *  The capacity of the signature buffer.
 *
 * @param [in|out] signature_length
 *   The length of the signature will be stored in that parameter.
 *
 * @param [in] privateKey
 *   An optional private key. If NULL is passed then private key will be derived using BIP32.
 *
 * @return
 *   1 if signature is verified
 *   0 is signature is not verified
 */
int sign_secp256k1(
        const uint8_t* message,
        unsigned int message_length,
        uint8_t* signature,
        unsigned int signature_capacity,
        unsigned int* signature_length,
        cx_ecfp_private_key_t* privateKey);
