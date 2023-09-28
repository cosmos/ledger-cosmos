#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "zxerror.h"

// https://github.com/debris/tiny-keccak
// Parameters are based on
// https://github.com/ethereum/solidity/blob/6bbedab383f7c8799ef7bcf4cad2bb008a7fcf2c/libdevcore/Keccak256.cpp

zxerr_t keccak_hash(const unsigned char *in, unsigned int inLen,
                    unsigned char *out, unsigned int outLen);

#ifdef __cplusplus
}
#endif
