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

#define BIP44_PATH_LEN (5 * sizeof(uin32_t))

void bip32_private(
        const uint32_t* bip32_path,
        size_t bip32_len,
        uint8_t privateKeyData[32])
{
    os_perso_derive_node_bip32(
            CX_CURVE_256K1,
            bip32_path,
            bip32_len,
            privateKeyData,
            NULL);
}

void keys_secp256k1(
        cx_ecfp_public_key_t* publicKey,
        cx_ecfp_private_key_t* privateKey,
        const uint8_t privateKeyData[32])
{
    cx_ecdsa_init_private_key(CX_CURVE_256K1, privateKeyData, 32, privateKey);
    cx_ecdsa_init_public_key(CX_CURVE_256K1, NULL, 0, publicKey);
    cx_ecfp_generate_pair(CX_CURVE_256K1, publicKey, privateKey, 1);
}

int sign_secp256k1(
        const uint8_t* message,
        unsigned int message_length,
        uint8_t* signature,
        unsigned int signature_capacity,
        unsigned int* signature_length,
        cx_ecfp_private_key_t* privateKey)
{
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

    // FIXME
    //    if (info & CX_ECCINFO_PARITY_ODD) {
    //        signature[0] |= 0x01;
    //    }
    //    return cx_ecdsa_verify(
    //            &publicKey,
    //            CX_LAST,
    //            CX_SHA256,
    //            message_digest,
    //            sizeof(message_digest),
    //            (unsigned char*) signature,
    //            *signature_length);

    return 1;
}

#ifdef ed25519
int generate_signature_ed25519(uint8_t *message, uint16_t message_length)
{
    return 0;
}
#endif
