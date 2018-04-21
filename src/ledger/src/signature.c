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
#include "signature.h"

#include "cx.h"

//
#define RESPONSE_BUFFER_LENGTH 100
#define DERIVATION_PATH_MAX_DEPTH 10

uint8_t response_buffer[RESPONSE_BUFFER_LENGTH];

unsigned int response_buffer_length = 0;

// derivation path = 44'/60'/0'/0/0
const uint32_t bip32_derivation_path[] =
    {
        0x80000000 | 44,
        0x80000000 | 60,
        0x80000000 | 0,
        0x80000000 | 0,
        0x80000000 | 0
    };

void generate_public_key_from_bip()
{
    // Generate keys
    uint8_t privateKeyData[32];
    os_perso_derive_node_bip32(
            CX_CURVE_256K1,
            bip32_derivation_path,
            sizeof(bip32_derivation_path) / sizeof(uint32_t),
            privateKeyData,
            NULL);

    cx_ecfp_private_key_t privateKey;
    cx_ecfp_init_private_key(
            CX_CURVE_256K1,
            privateKeyData,
            32,
            &privateKey);

    cx_ecfp_public_key_t publicKey;
    cx_ecfp_generate_pair(CX_CURVE_256K1,
                          &publicKey,
                          &privateKey,
                          1);

    os_memmove(response_buffer, publicKey.W, 65);
    response_buffer_length = 65;
}

int generate_signature_SECP256K1(uint8_t *message, uint16_t message_length)
{
    uint8_t message_digest[CX_SHA256_SIZE];
    cx_hash_sha256(message, message_length, message_digest, CX_SHA256_SIZE);

    // Generate keys
    uint8_t privateKeyData[32];
    os_perso_derive_node_bip32(
        CX_CURVE_256K1,
        bip32_derivation_path,
        sizeof(bip32_derivation_path) / sizeof(uint32_t),
        privateKeyData,
        NULL);

    cx_ecfp_private_key_t privateKey;
    cx_ecfp_init_private_key(
        CX_CURVE_256K1,
        privateKeyData,
        32,
        &privateKey);

    cx_ecfp_public_key_t publicKey;
    cx_ecfp_generate_pair(CX_CURVE_256K1,
                          &publicKey,
                          &privateKey,
                          1);

    unsigned int info = 0;

    // reset signature
    os_memset((void *) response_buffer, 0, sizeof(response_buffer));
    response_buffer_length = 0;

    // sign
    response_buffer_length = cx_ecdsa_sign(&privateKey,
                           CX_RND_RFC6979 | CX_LAST,
                           CX_SHA256,
                           message_digest,
                           sizeof(message_digest),
                           response_buffer,
                           sizeof(response_buffer),
                           &info);

    if (info & CX_ECCINFO_PARITY_ODD) {
        response_buffer[0] = 0x01;
    }
    os_memset(&privateKey, 0, sizeof(privateKey));

    return cx_ecdsa_verify(&publicKey,
                           CX_LAST,
                           CX_SHA256,
                           message_digest,
                           sizeof(message_digest),
                           (unsigned char *) response_buffer,
                           response_buffer_length);
}

int generate_signature_ED25519(uint8_t *message, uint16_t message_length)
{
//    uint8_t message_digest[CX_SHA512_SIZE];
//    cx_hash_sha512(message, message_length, message_digest, CX_SHA512_SIZE);
//
//    // Reset signature
//    os_memset((void *) signature, 0, sizeof(signature));
//    length = 0;
//
//    uint8_t privateKeyData[32];
//    cx_ecfp_private_key_t privateKey;
//    cx_ecfp_public_key_t publicKey;
//
//    os_perso_derive_node_bip32(
//        CX_CURVE_Ed25519,
//        bip32_derivation_path,
//        sizeof(bip32_derivation_path) / sizeof(uint32_t),
//        privateKeyData,
//        NULL);
//
//    cx_ecfp_init_private_key(
//        CX_CURVE_Ed25519,
//        privateKeyData,
//        32,
//        &privateKey);
//
//    os_memset(privateKeyData, 0, sizeof(privateKeyData));
//
//    cx_ecfp_generate_pair(CX_CURVE_Ed25519,
//                          &publicKey,
//                          &privateKey,
//                          1);
//
//    unsigned int info = 0;
//
//    length = cx_eddsa_sign(&privateKey,
//                           0,
//                           CX_SHA512,
//                           message_digest,
//                           sizeof(message_digest),
//                           NULL,
//                           0,
//                           signature,
//                           sizeof(signature),
//                           &info);
//
//    if (info & CX_ECCINFO_PARITY_ODD) {
//        signature[0] = 0x01;
//    }
//    os_memset(&privateKey, 0, sizeof(privateKey));
//
//    return cx_eddsa_verify(&publicKey,
//                           0,
//                           CX_SHA512,
//                           message_digest,
//                           sizeof(message_digest),
//                           NULL,
//                           0,
//                           (unsigned char *) signature,
//                           length);
    return 1;
}

uint8_t* get_response_buffer()
{
    return response_buffer;
}

unsigned int get_response_buffer_length()
{
    return response_buffer_length;
}
