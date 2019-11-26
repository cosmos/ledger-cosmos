/*******************************************************************************
*   (c) 2019 ZondaX GmbH
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

#include "crypto.h"
#include "coin.h"

#include <bech32.h>
#include "apdu_codes.h"
#include "zxmacros.h"

uint32_t bip44Path[BIP44_LEN_DEFAULT];
uint8_t bech32_hrp_len;
char bech32_hrp[MAX_BECH32_HRP_LEN + 1];

#if defined(TARGET_NANOS)
#define SAFE_HEARTBEAT(X)  io_seproxyhal_io_heartbeat(); X; io_seproxyhal_io_heartbeat();
#endif

#if defined(TARGET_NANOX)
#define SAFE_HEARTBEAT(X)  X;
#endif

#if defined(TARGET_NANOS) || defined(TARGET_NANOX)
#include "cx.h"

void crypto_extractPublicKey(uint32_t bip44Path[BIP44_LEN_DEFAULT], uint8_t *pubKey){
    cx_ecfp_public_key_t cx_publicKey;
    cx_ecfp_private_key_t cx_privateKey;
    uint8_t privateKeyData[32];

    BEGIN_TRY
    {
        TRY {
            SAFE_HEARTBEAT(os_perso_derive_node_bip32(CX_CURVE_256K1,
                                                      bip44Path,
                                                      BIP44_LEN_DEFAULT,
                                                      privateKeyData, NULL));

            //////////////////////
            SAFE_HEARTBEAT(cx_ecfp_init_private_key(CX_CURVE_256K1, privateKeyData, 32, &cx_privateKey));
            SAFE_HEARTBEAT(cx_ecfp_init_public_key(CX_CURVE_256K1, NULL, 0, &cx_publicKey));
            SAFE_HEARTBEAT(cx_ecfp_generate_pair(CX_CURVE_256K1, &cx_publicKey, &cx_privateKey, 1));
        }
        FINALLY {
            MEMZERO(&cx_privateKey, sizeof(cx_privateKey));
            MEMZERO(privateKeyData, 32);
        }
    }
    END_TRY;

    // Format pubkey
    for (int i = 0; i < 32; i++) {
        pubKey[i] = cx_publicKey.W[64 - i];
    }
    cx_publicKey.W[0] = cx_publicKey.W[64] & 1 ? 0x03 : 0x02; // "Compress" public key in place
    if ((cx_publicKey.W[32] & 1) != 0) {
        pubKey[31] |= 0x80;
    }
    //////////////////////
    memcpy(pubKey, cx_publicKey.W, PK_LEN);
}

uint16_t crypto_sign(uint8_t *signature, uint16_t signatureMaxlen, const uint8_t *message, uint16_t messageLen) {
    uint8_t message_digest[CX_SHA256_SIZE];
    SAFE_HEARTBEAT(cx_hash_sha256(message, messageLen, message_digest, CX_SHA256_SIZE));

    cx_ecfp_private_key_t cx_privateKey;
    uint8_t privateKeyData[32];
    int signatureLength;
    BEGIN_TRY
    {
        TRY
        {
            // Generate keys
            SAFE_HEARTBEAT(os_perso_derive_node_bip32(CX_CURVE_256K1,
                                                      bip44Path,
                                                      BIP44_LEN_DEFAULT,
                                                      privateKeyData, NULL));

            SAFE_HEARTBEAT(cx_ecfp_init_private_key(CX_CURVE_256K1, privateKeyData, 32, &cx_privateKey));

            // Sign
            unsigned int info = 0;
            SAFE_HEARTBEAT(
                signatureLength = cx_ecdsa_sign(&cx_privateKey,
                                                CX_RND_RFC6979 | CX_LAST,
                                                CX_SHA256,
                                                message_digest,
                                                CX_SHA256_SIZE,
                                                signature,
                                                signatureMaxlen,
                                                &info));
        }
        FINALLY {
            MEMZERO(&cx_privateKey, sizeof(cx_privateKey));
            MEMZERO(privateKeyData, 32);
        }
    }
    END_TRY;

    return signatureLength;
}
#else

void crypto_extractPublicKey(uint32_t path[BIP32_LEN_DEFAULT], uint8_t *pubKey) {
    // Empty version for non-Ledger devices
    MEMZERO(pubKey, 32);
}

uint16_t crypto_sign(uint8_t *signature,
                     uint16_t signatureMaxlen,
                     const uint8_t *message,
                     uint16_t messageLen) {
    // Empty version for non-Ledger devices
    return 0;
}

#endif

uint8_t extractHRP(uint32_t rx, uint32_t offset) {
    if (rx < offset + 1) {
        THROW(APDU_CODE_DATA_INVALID);
    }
    MEMZERO(bech32_hrp, MAX_BECH32_HRP_LEN);

    bech32_hrp_len = G_io_apdu_buffer[offset];

    if (bech32_hrp_len == 0 || bech32_hrp_len > MAX_BECH32_HRP_LEN) {
        THROW(APDU_CODE_DATA_INVALID);
    }

    memcpy(bech32_hrp, G_io_apdu_buffer + offset + 1, bech32_hrp_len);
    bech32_hrp[bech32_hrp_len] = 0;     // zero terminate

    return bech32_hrp_len;
}

void ripemd160_32(uint8_t *out, uint8_t *in) {
    cx_ripemd160_t rip160;
    cx_ripemd160_init(&rip160);
    cx_hash(&rip160.header, CX_LAST, in, CX_SHA256_SIZE, out, CX_RIPEMD160_SIZE);
}

void crypto_set_hrp(char *p) {
    bech32_hrp_len = strlen(p);
    if (bech32_hrp_len < MAX_BECH32_HRP_LEN) {
        strcpy(bech32_hrp, p);
    }
}

uint16_t crypto_fillAddress(uint8_t *buffer, uint16_t buffer_len) {
    if (buffer_len < PK_LEN + 50) {
        return 0;
    }

    // extract pubkey
    crypto_extractPublicKey(bip44Path, buffer);

    // Hash it
    uint8_t hashed1_pk[CX_SHA256_SIZE];
    cx_hash_sha256(buffer, PK_LEN, hashed1_pk, CX_SHA256_SIZE);

    uint8_t hashed2_pk[CX_RIPEMD160_SIZE];
    ripemd160_32(hashed2_pk, hashed1_pk);

    char *addr = (char *) (buffer + PK_LEN);
    bech32EncodeFromBytes(addr, bech32_hrp, hashed2_pk, CX_RIPEMD160_SIZE);

    return PK_LEN + strlen(addr);
}
