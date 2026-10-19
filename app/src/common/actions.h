/*******************************************************************************
 *   (c) 2019 Zondax GmbH
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

#include "apdu_codes.h"
#include "coin.h"
#include "crypto.h"
#include "tx.h"
#include "zxerror.h"
#include <os_io_seproxyhal.h>
#include <stdint.h>

typedef enum {
  TX_STATE_IDLE = 0,
  TX_STATE_RECEIVING,
  TX_STATE_REVIEWING,
} tx_state_e;

extern tx_state_e g_tx_state;

// The answer to a GetAddr request, kept in app RAM rather than in
// G_io_apdu_buffer.
//
// G_io_apdu_buffer is where the next incoming APDU lands, and app_main's
// io_exchange hands one over as soon as it arrives -- which, once a review is
// on screen and the handler has returned IO_ASYNCH_REPLY, is while the user is
// still reading. An address paged out of that buffer at draw time, or replied
// out of it at approve time, is then whatever the host sent last instead of
// what the user was shown and agreed to.
//
// Sized for the longest response crypto_fillAddress can produce: the
// compressed public key, then the address text -- bech32 with the longest HRP
// the app accepts, or the hex form the ETH encoding uses -- and a terminator.
#define ACTION_ADDR_RESPONSE_SIZE                                              \
  (PK_LEN_SECP256K1 + MAX_BECH32_HRP_LEN + MIN_ADDRESS_BUFFER_SPACE)

extern uint8_t action_addrResponse[ACTION_ADDR_RESPONSE_SIZE];
extern uint16_t action_addrResponseLen;

__Z_INLINE void app_sign() {
  uint16_t replyLen = 0;

  MEMZERO(G_io_apdu_buffer, IO_APDU_BUFFER_SIZE);
  const zxerr_t err =
      crypto_sign(G_io_apdu_buffer, IO_APDU_BUFFER_SIZE - 3, &replyLen);

  g_tx_state = TX_STATE_IDLE;

  if (err != zxerr_ok || replyLen == 0) {
    set_code(G_io_apdu_buffer, 0, APDU_CODE_SIGN_VERIFY_ERROR);
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
  } else {
    set_code(G_io_apdu_buffer, replyLen, APDU_CODE_OK);
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, replyLen + 2);
  }
}

__Z_INLINE zxerr_t app_fill_address() {
  // Into app RAM, so the address the review draws and the address the reply
  // carries are the ones this request derived (see action_addrResponse).
  MEMZERO(action_addrResponse, sizeof(action_addrResponse));

  action_addrResponseLen = 0;
  const zxerr_t err =
      crypto_fillAddress(action_addrResponse, sizeof(action_addrResponse),
                         &action_addrResponseLen);

  if (err != zxerr_ok || action_addrResponseLen == 0) {
    MEMZERO(action_addrResponse, sizeof(action_addrResponse));
    action_addrResponseLen = 0;
    THROW(APDU_CODE_EXECUTION_ERROR);
  }

  return zxerr_ok;
}

// Copies the held answer into the APDU buffer, which by now may hold an
// unrelated request the host sent while the review was up.
__Z_INLINE void app_load_address_response() {
  MEMZERO(G_io_apdu_buffer, IO_APDU_BUFFER_SIZE);
  MEMCPY(G_io_apdu_buffer, action_addrResponse, action_addrResponseLen);
}

__Z_INLINE void app_reject() {
  g_tx_state = TX_STATE_IDLE;
  MEMZERO(G_io_apdu_buffer, IO_APDU_BUFFER_SIZE);
  set_code(G_io_apdu_buffer, 0, APDU_CODE_COMMAND_NOT_ALLOWED);
  io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
}

__Z_INLINE void app_reply_address() {
  g_tx_state = TX_STATE_IDLE;
  app_load_address_response();
  set_code(G_io_apdu_buffer, action_addrResponseLen, APDU_CODE_OK);
  io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, action_addrResponseLen + 2);
}

__Z_INLINE void app_reply_error() {
  g_tx_state = TX_STATE_IDLE;
  set_code(G_io_apdu_buffer, 0, APDU_CODE_DATA_INVALID);
  io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
}
