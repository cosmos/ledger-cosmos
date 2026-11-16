/*******************************************************************************
 *   (c) 2026 Zondax AG
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
#include "swap_host_stubs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Nothing to wipe on the host.
void os_explicit_zero_BSS_segment(void) {}

// Returns to Exchange on the device; the tests never get that far.
void os_lib_end(void) { abort(); }

// Models what matters to check_address: which paths the OS lets the app derive
// (PATH_APP_LOAD_PARAMS in app/Makefile). The address is a deterministic,
// path-dependent "<hrp>1" + hex(account, change, index) instead of a real key.
zxerr_t crypto_swap_fillAddress(uint32_t *hdPath_swap, uint8_t hdPathLen_swap,
                                char *hrp, address_encoding_e encode_type,
                                char *buffer, uint16_t bufferLen,
                                uint16_t *addrResponseLen) {
  (void)encode_type;
  if (hdPath_swap == NULL || hrp == NULL || buffer == NULL ||
      addrResponseLen == NULL || hdPathLen_swap != 5) {
    return zxerr_unknown;
  }
  const uint32_t purpose = hdPath_swap[0];
  const uint32_t coin = hdPath_swap[1];
  if (purpose != 0x8000002Cu ||
      (coin != 0x80000076u && coin != 0x8000003Cu && coin != 0x800004B0u)) {
    return zxerr_unknown;
  }
  const int n = snprintf(buffer, bufferLen, "%s1%08x%08x%08x", hrp,
                         hdPath_swap[2], hdPath_swap[3], hdPath_swap[4]);
  if (n < 0 || n >= bufferLen) {
    return zxerr_buffer_too_small;
  }
  *addrResponseLen = (uint16_t)n;
  return zxerr_ok;
}
