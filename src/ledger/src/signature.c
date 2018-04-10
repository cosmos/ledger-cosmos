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

#define SIGNATURE_LENGTH 100
#define DERIVATION_PATH_MAX_DEPTH 10
uint8_t signature[SIGNATURE_LENGTH];
uint32_t length = 0;
uint32_t bip32_derivation_path[DERIVATION_PATH_MAX_DEPTH];
uint8_t bip32_derivation_path_length;

void signature_reset_SECP256K1()
{
    os_memset((void*)signature, 0, sizeof(signature));
    length = 0;
    // derivation path = 44'/60'/0'/0/0
    bip32_derivation_path[0] = 0x80000000 | 44 ;
    bip32_derivation_path[1] = 0x80000000 | 60;
    bip32_derivation_path[2] = 0x80000000 | 0;
    bip32_derivation_path[3] = 0;
    bip32_derivation_path[4] = 0;
    bip32_derivation_path_length = 5;
}


void signature_reset_ED25519()
{
    os_memset((void*)signature, 0, sizeof(signature));
    length = 0;
    // derivation path = 44'/60'/0'/0'/0'
    bip32_derivation_path[0] = 0x80000000 | 44 ;
    bip32_derivation_path[1] = 0x80000000 | 60;
    bip32_derivation_path[2] = 0x80000000 | 0;
    bip32_derivation_path[3] = 0x80000000 | 0;
    bip32_derivation_path[4] = 0x80000000 | 0;
    bip32_derivation_path_length = 5;
}

void signature_set_derivation_path(uint32_t* path, uint32_t path_size)
{
    os_memmove(bip32_derivation_path, path, path_size*sizeof(path[0]));
    bip32_derivation_path_length = path_size;
}

int signature_create_SECP256K1(uint8_t* message, uint16_t message_length) {
    cx_sha256_t sha256;
    uint8_t hashMessage[32];

    signature_reset_SECP256K1();

    cx_sha256_init(&sha256);

    cx_hash((cx_hash_t *) &sha256, 0, message, message_length, NULL);
    cx_hash((cx_hash_t *) &sha256, CX_LAST, message, 0, hashMessage);

    uint8_t privateKeyData[32];
    cx_ecfp_private_key_t privateKey;
    cx_ecfp_public_key_t publicKey;

    os_perso_derive_node_bip32(
            CX_CURVE_256K1,
            bip32_derivation_path,
            bip32_derivation_path_length,
            privateKeyData,
            NULL);

    cx_ecfp_init_private_key(
            CX_CURVE_256K1,
            privateKeyData,
            32,
            &privateKey);

    os_memset(privateKeyData, 0, sizeof(privateKeyData));

    cx_ecfp_generate_pair(CX_CURVE_256K1,
                          &publicKey,
                          &privateKey,
                          1);

    unsigned int info = 0;

    length = cx_ecdsa_sign(&privateKey,
                           CX_RND_RFC6979 | CX_LAST,
                           CX_SHA256,
                           hashMessage,
                           sizeof(hashMessage),
                           signature,
                           &info);

    if (info & CX_ECCINFO_PARITY_ODD) {
        signature[0] = 0x01;
    }
    os_memset(&privateKey, 0, sizeof(privateKey));

    return cx_ecdsa_verify(&publicKey,
                           CX_LAST,
                           CX_SHA256,
                           hashMessage,
                           sizeof(hashMessage),
                           (unsigned char*)signature,
                           length);
}

int signature_create_ED25519(uint8_t* message, uint16_t message_length) {
    cx_sha512_t sha512;
    uint8_t hashMessage[64];

    signature_reset_ED25519();

    cx_sha512_init(&sha512);

    cx_hash((cx_hash_t *) &sha512, 0, message, message_length, NULL);
    cx_hash((cx_hash_t *) &sha512, CX_LAST, message, 0, hashMessage);

    uint8_t privateKeyData[32];
    cx_ecfp_private_key_t privateKey;
    cx_ecfp_public_key_t publicKey;

    os_perso_derive_node_bip32(
            CX_CURVE_Ed25519,
            bip32_derivation_path,
            bip32_derivation_path_length,
            privateKeyData,
            NULL);

    cx_ecfp_init_private_key(
            CX_CURVE_Ed25519,
            privateKeyData,
            32,
            &privateKey);

    os_memset(privateKeyData, 0, sizeof(privateKeyData));

    cx_ecfp_generate_pair(CX_CURVE_Ed25519,
                          &publicKey,
                          &privateKey,
                          1);

    unsigned int info = 0;

    length = cx_eddsa_sign(&privateKey,
                           0,
                           CX_SHA512,
                           hashMessage,
                           sizeof(hashMessage),
                           NULL,
                           0,
                           signature,
                           &info);

    if (info & CX_ECCINFO_PARITY_ODD) {
        signature[0] = 0x01;
    }
    os_memset(&privateKey, 0, sizeof(privateKey));

    return cx_eddsa_verify(&publicKey,
                           0,
                           CX_SHA512,
                           hashMessage,
                           sizeof(hashMessage),
                           NULL,
                           0,
                           (unsigned char*)signature,
                           length);
}

uint8_t* signature_get()
{
    return signature;
}

uint32_t signature_length()
{
    return length;
}
