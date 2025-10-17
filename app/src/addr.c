/*******************************************************************************
 *   (c) 2020 Zondax GmbH
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

#include "app_mode.h"
#include "coin.h"
#include "crypto.h"
#include "zxerror.h"
#include "zxformat.h"
#include "zxmacros.h"
#include <stdio.h>

zxerr_t addr_getNumItems(uint8_t *num_items) {
  zemu_log_stack("addr_getNumItems");
  *num_items = 1;

  if (app_mode_expert() || encoding != BECH32_COSMOS) {
    zemu_log("num_items 2\n");
    *num_items = 2;
  } else {
    zemu_log("num_items 1\n");
  }
  return zxerr_ok;
}

zxerr_t addr_getItem(int8_t displayIdx, char *outKey, uint16_t outKeyLen,
                     char *outVal, uint16_t outValLen, uint8_t pageIdx,
                     uint8_t *pageCount) {
  ZEMU_LOGF(200, "[addr_getItem] %d/%d\n", displayIdx, pageIdx)

  switch (displayIdx) {
  case 0:
    snprintf(outKey, outKeyLen, "Address");
    pageString(outVal, outValLen,
               (char *)(G_io_apdu_buffer + VIEW_ADDRESS_OFFSET_SECP256K1),
               pageIdx, pageCount);
    ZEMU_LOGF(200, "[addr_getItem] pageCount %d\n", *pageCount)
    return zxerr_ok;
  case 1: {
    if (!app_mode_expert() && encoding == BECH32_COSMOS) {
      return zxerr_no_data;
    }

    char buffer[300];
    snprintf(outKey, outKeyLen, "Path");
    bip32_to_str(buffer, sizeof(buffer), hdPath, HDPATH_LEN_DEFAULT);
    pageString(outVal, outValLen, buffer, pageIdx, pageCount);
    ZEMU_LOGF(200, "[addr_getItem] pageCount %d\n", *pageCount)
    return zxerr_ok;
  }
  default:
    zemu_log("[addr_getItem] no data\n");
    return zxerr_no_data;
  }
}
