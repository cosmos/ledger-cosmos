/*******************************************************************************
 *   (c) 2018 - 2023 Zondax AG
 *   (c) 2016 Ledger
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

#include "app_main.h"

#include <os.h>
#include <os_io_seproxyhal.h>
#include <string.h>
#include <ux.h>

#include "actions.h"
#include "addr.h"
#include "app_mode.h"
#include "coin.h"
#include "crypto.h"
#include "tx.h"
#include "view.h"
#include "view_internal.h"
#include "zxmacros.h"

#include "parser_impl.h"
#include "view_internal.h"

#include "chain_config.h"

#ifdef HAVE_SWAP
#include "swap.h"
#endif

static const char *msg_error1 = "Expert Mode";
static const char *msg_error2 = "Required";

__Z_INLINE void handle_getversion(__Z_UNUSED volatile uint32_t *flags,
                                  volatile uint32_t *tx,
                                  __Z_UNUSED uint32_t rx) {
#ifdef DEBUG
  G_io_apdu_buffer[0] = 0xFF;
#else
  G_io_apdu_buffer[0] = 0;
#endif
  G_io_apdu_buffer[1] = MAJOR_VERSION;
  G_io_apdu_buffer[2] = MINOR_VERSION;
  G_io_apdu_buffer[3] = PATCH_VERSION;
  // SDK won't let the app reply an apdu message if screensaver is active
  // device_locked field --> Always false
  G_io_apdu_buffer[4] = 0;

  G_io_apdu_buffer[5] = (TARGET_ID >> 24) & 0xFF;
  G_io_apdu_buffer[6] = (TARGET_ID >> 16) & 0xFF;
  G_io_apdu_buffer[7] = (TARGET_ID >> 8) & 0xFF;
  G_io_apdu_buffer[8] = (TARGET_ID >> 0) & 0xFF;

  *tx += 9;
  THROW(APDU_CODE_OK);
}

__Z_INLINE uint8_t extractHRP(uint32_t rx, uint32_t offset) {
  uint8_t hrp_len = 0;
  if (rx < offset + 1) {
    THROW(APDU_CODE_DATA_INVALID);
  }
  MEMZERO(bech32_hrp, MAX_BECH32_HRP_LEN);

  bech32_hrp_len = G_io_apdu_buffer[offset];

  if (bech32_hrp_len == 0 || bech32_hrp_len > MAX_BECH32_HRP_LEN) {
    THROW(APDU_CODE_HRP_WRONG_LENGTH);
  }

  // Verify buffer contains full HRP data to prevent buffer over-read
  if (rx < offset + 1 + bech32_hrp_len) {
    THROW(APDU_CODE_DATA_INVALID);
  }

  memcpy(bech32_hrp, G_io_apdu_buffer + offset + 1, bech32_hrp_len);
  bech32_hrp[bech32_hrp_len] = 0; // zero terminate

  hrp_len = bech32_hrp_len;
  return hrp_len;
}

__Z_INLINE void extractHDPath(uint32_t rx, uint32_t offset) {
  if (rx < offset + 1) {
    THROW(APDU_CODE_DATA_INVALID);
  }

  if ((rx - offset) < sizeof(uint32_t) * HDPATH_LEN_DEFAULT) {
    THROW(APDU_CODE_WRONG_LENGTH);
  }

  MEMCPY(hdPath, G_io_apdu_buffer + offset,
         sizeof(uint32_t) * HDPATH_LEN_DEFAULT);

  // Check values
  if (hdPath[0] != HDPATH_0_DEFAULT ||
      ((hdPath[1] != HDPATH_1_DEFAULT) &&
       (hdPath[1] != HDPATH_ETH_1_DEFAULT)) ||
      hdPath[3] != HDPATH_3_DEFAULT) {
    THROW(APDU_CODE_INVALID_HD_PATH_COIN_VALUE);
  }

  // Limit values unless the app is running in expert mode
  if (!app_mode_expert()) {
    for (int i = 2; i < HDPATH_LEN_DEFAULT; i++) {
      // hardened or unhardened values should be below 20
      if ((hdPath[i] & 0x7FFFFFFF) > 100)
        THROW(APDU_CODE_INVALID_HD_PATH_VALUE);
    }
  }
}

static void extractHDPath_HRP(uint32_t rx, uint32_t offset) {
  extractHDPath(rx, offset);
  // Set BECH32_COSMOS as default for backward compatibility
  encoding = BECH32_COSMOS;

  // Check if HRP was sent
  if ((rx - offset) > sizeof(uint32_t) * HDPATH_LEN_DEFAULT) {
    uint8_t hrp_bech32_len =
        extractHRP(rx, offset + sizeof(uint32_t) * HDPATH_LEN_DEFAULT);
    encoding = checkChainConfig(hdPath[1], bech32_hrp, hrp_bech32_len);
    if (encoding == UNSUPPORTED) {
      ZEMU_LOGF(50, "Chain config not supported for: %s\n", bech32_hrp)
      THROW(APDU_CODE_CHAIN_CONFIG_NOT_SUPPORTED);
    }
  } else if (hdPath[1] == HDPATH_ETH_1_DEFAULT) {
    THROW(APDU_CODE_INVALID_HD_PATH_COIN_VALUE);
  }
}

static bool process_chunk(volatile uint32_t *tx, uint32_t rx) {
  UNUSED(tx);

  const uint8_t payloadType = G_io_apdu_buffer[OFFSET_PAYLOAD_TYPE];

  if (rx < OFFSET_DATA) {
    THROW(APDU_CODE_WRONG_LENGTH);
  }

  uint32_t added;
  switch (payloadType) {
  case P1_INIT:
    tx_initialize();
    tx_reset();
    extractHDPath_HRP(rx, OFFSET_DATA);
    return false;
  case P1_ADD:
    added = tx_append(&(G_io_apdu_buffer[OFFSET_DATA]), rx - OFFSET_DATA);
    if (added != rx - OFFSET_DATA) {
      THROW(APDU_CODE_TRANSACTION_DATA_EXCEEDS_BUFFER_CAPACITY);
    }
    return false;
  case P1_LAST:
    added = tx_append(&(G_io_apdu_buffer[OFFSET_DATA]), rx - OFFSET_DATA);
    if (added != rx - OFFSET_DATA) {
      THROW(APDU_CODE_TRANSACTION_DATA_EXCEEDS_BUFFER_CAPACITY);
    }
    return true;
  }

  THROW(APDU_CODE_INVALIDP1P2);
  return false;
}

__Z_INLINE void handleGetAddrSecp256K1(volatile uint32_t *flags,
                                       volatile uint32_t *tx, uint32_t rx) {
  uint8_t len = extractHRP(rx, OFFSET_DATA);
  extractHDPath(rx, OFFSET_DATA + 1 + len);

  // Verify encoding
  encoding = checkChainConfig(hdPath[1], bech32_hrp, bech32_hrp_len);
  if (encoding == UNSUPPORTED) {
    ZEMU_LOGF(50, "Chain config not supported for: %s\n", bech32_hrp)
    THROW(APDU_CODE_CHAIN_CONFIG_NOT_SUPPORTED);
  }

  uint8_t requireConfirmation = G_io_apdu_buffer[OFFSET_P1];
  zxerr_t zxerr = app_fill_address();
  if (zxerr != zxerr_ok) {
    *tx = 0;
    THROW(APDU_CODE_DATA_INVALID);
  }

  if (requireConfirmation) {
    view_review_init(addr_getItem, addr_getNumItems, app_reply_address);
    view_review_show(REVIEW_ADDRESS);
    *flags |= IO_ASYNCH_REPLY;
    return;
  }

  *tx = action_addrResponseLen;
  THROW(APDU_CODE_OK);
}

__Z_INLINE void handleSign(volatile uint32_t *flags, volatile uint32_t *tx,
                           uint32_t rx) {
  if (!process_chunk(tx, rx)) {
    THROW(APDU_CODE_OK);
  }

  // Let grab P2 value and if it's not valid, the parser should reject it
  const tx_type_e sign_type = (tx_type_e)G_io_apdu_buffer[OFFSET_P2];

  if ((hdPath[1] == HDPATH_ETH_1_DEFAULT) && !app_mode_expert()) {
    *flags |= IO_ASYNCH_REPLY;
    view_custom_error_show(PIC(msg_error1), PIC(msg_error2));
    THROW(APDU_CODE_DATA_INVALID);
  }

  parser_tx_obj.tx_json.own_addr =
      (const char *)(G_io_apdu_buffer + VIEW_ADDRESS_OFFSET_SECP256K1);
  const char *error_msg = tx_parse(sign_type);
  if (error_msg != NULL) {
    const int error_msg_length = strnlen(error_msg, sizeof(G_io_apdu_buffer));
    MEMCPY(G_io_apdu_buffer, error_msg, error_msg_length);
    *tx += (error_msg_length);
    THROW(APDU_CODE_DATA_INVALID);
  }

#ifdef HAVE_SWAP
  if (G_swap_state.called_from_swap && G_swap_state.should_exit &&
      error_msg == NULL) {
    // Call app_sign without going through UI display, the UI validation was
    // done in Exchange app already
    app_sign();
    // Go back to Exchange and report our success to display the modal
    finalize_exchange_sign_transaction(true);
    // Unreachable
  }
#endif

  CHECK_APP_CANARY()
  view_review_init(tx_getItem, tx_getNumItems, app_sign);
  view_review_show(REVIEW_TXN);
  *flags |= IO_ASYNCH_REPLY;
}

void handleApdu(volatile uint32_t *flags, volatile uint32_t *tx, uint32_t rx) {
  volatile uint16_t sw = 0;

  BEGIN_TRY {
    TRY {
      if (G_io_apdu_buffer[OFFSET_CLA] != CLA) {
        THROW(APDU_CODE_CLA_NOT_SUPPORTED);
      }

      if (rx < APDU_MIN_LENGTH) {
        THROW(APDU_CODE_WRONG_LENGTH);
      }

      switch (G_io_apdu_buffer[OFFSET_INS]) {
      case INS_GET_VERSION: {
        handle_getversion(flags, tx, rx);
        break;
      }

      case INS_GET_ADDR_SECP256K1: {
        CHECK_PIN_VALIDATED()
        handleGetAddrSecp256K1(flags, tx, rx);
        break;
      }

      case INS_SIGN_SECP256K1: {
        CHECK_PIN_VALIDATED()
        handleSign(flags, tx, rx);
        break;
      }

      default:
        THROW(APDU_CODE_INS_NOT_SUPPORTED);
      }
    }
    CATCH(EXCEPTION_IO_RESET) { THROW(EXCEPTION_IO_RESET); }
    CATCH_OTHER(e) {
      switch (e & 0xF000) {
      case 0x6000:
      case APDU_CODE_OK:
        sw = e;
        break;
      default:
        sw = 0x6800 | (e & 0x7FF);
        break;
      }
      G_io_apdu_buffer[*tx] = sw >> 8;
      G_io_apdu_buffer[*tx + 1] = sw & 0xFF;
      *tx += 2;
    }
    FINALLY {
#ifdef HAVE_SWAP
      if (G_swap_state.called_from_swap && G_swap_state.should_exit) {
        // Swap checking failed, send reply now and exit, don't wait next cycle
        if (sw != 0) {
          io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, *tx);
        }
        // Go back to exchange and report our status
        finalize_exchange_sign_transaction(sw == 0);
      }
#endif
    }
  }
  END_TRY;
}
