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
#include "apdu_codes.h"

void keys_secp256k1(
        cx_ecfp_public_key_t *publicKey,
        cx_ecfp_private_key_t *privateKey,
        const uint8_t privateKeyData[32]) {
    cx_ecfp_init_private_key(CX_CURVE_256K1, privateKeyData, 32, privateKey);
    cx_ecfp_init_public_key(CX_CURVE_256K1, NULL, 0, publicKey);
    cx_ecfp_generate_pair(CX_CURVE_256K1, publicKey, privateKey, 1);
}

void keys_ed25519(
        cx_ecfp_public_key_t *publicKey,
        cx_ecfp_private_key_t *privateKey,
        const uint8_t privateKeyData[32]) {
    cx_ecfp_init_private_key(CX_CURVE_Ed25519, privateKeyData, 32, privateKey);
    cx_ecfp_init_public_key(CX_CURVE_Ed25519, NULL, 0, publicKey);
    cx_ecfp_generate_pair(CX_CURVE_Ed25519, publicKey, privateKey, 1);
}

int sign_secp256k1(
        const uint8_t *message,
        unsigned int message_length,
        uint8_t *signature,
        unsigned int signature_capacity,
        unsigned int *signature_length,
        cx_ecfp_private_key_t *privateKey) {
    uint8_t message_digest[CX_SHA256_SIZE];
    cx_hash_sha256(message, message_length, message_digest, CX_SHA256_SIZE);

    cx_ecfp_public_key_t publicKey;
    cx_ecdsa_init_public_key(CX_CURVE_256K1, NULL, 0, &publicKey);
    cx_ecfp_generate_pair(CX_CURVE_256K1, &publicKey, privateKey, 1);

    unsigned int info = 0;
    *signature_length = cx_ecdsa_sign(
            privateKey,
            CX_RND_RFC6979 | CX_LAST,
            CX_SHA256,
            message_digest,
            CX_SHA256_SIZE,
            signature,
            signature_capacity,
            &info);

    os_memset(&privateKey, 0, sizeof(privateKey));
#ifdef TESTING_ENABLED
    return cx_ecdsa_verify(
            &publicKey,
            CX_LAST,
            CX_SHA256,
            message_digest,
            CX_SHA256_SIZE,
            signature,
            *signature_length);
#else
    return 1;
#endif
}

int sign_ed25519(
        const uint8_t *message,
        unsigned int message_length,
        uint8_t *signature,
        unsigned int signature_capacity,
        unsigned int *signature_length,
        cx_ecfp_private_key_t *privateKey) {
    uint8_t message_digest[CX_SHA512_SIZE];
    cx_hash_sha512(message, message_length, message_digest, CX_SHA512_SIZE);

    cx_ecfp_public_key_t publicKey;
    cx_ecfp_init_public_key(CX_CURVE_Ed25519, NULL, 0, &publicKey);
    cx_ecfp_generate_pair(CX_CURVE_Ed25519, &publicKey, privateKey, 1);

    unsigned int info = 0;

    *signature_length = cx_eddsa_sign(
            privateKey,
            CX_LAST,
            CX_SHA512,
            message_digest,
            CX_SHA512_SIZE,
            NULL,
            0,
            signature,
            signature_capacity,
            &info);

    os_memset(privateKey, 0, sizeof(privateKey));

#ifdef TESTING_ENABLED
    return cx_eddsa_verify(
            &publicKey,
            0,
            CX_SHA512,
            message_digest,
            CX_SHA512_SIZE,
            NULL,
            0,
            signature,
            *signature_length);
#else
    return 1;
#endif
}
