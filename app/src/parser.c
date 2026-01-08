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

#include "common/parser.h"
#include "app_mode.h"
#include "coin.h"
#include "parser_impl.h"
#include "tx_display.h"
#include "tx_parser.h"
#include <cbor/cbor_parser_helper.h>
#include <stdio.h>
#include <tx_validate.h>
#include <zxformat.h>
#include <zxmacros.h>
#include <zxtypes.h>

parser_error_t parser_init_context(parser_context_t *ctx, const uint8_t *buffer,
                                   uint16_t bufferSize) {
  if (ctx == NULL) {
    return parser_unexpected_value;
  }

  ctx->offset = 0;

  if (bufferSize == 0 || buffer == NULL) {
    // Not available, use defaults
    ctx->buffer = NULL;
    ctx->bufferLen = 0;
    return parser_init_context_empty;
  }

  ctx->buffer = buffer;
  ctx->bufferLen = bufferSize;

  return parser_ok;
}

parser_error_t parser_parse(parser_context_t *ctx, const uint8_t *data,
                            size_t dataLen, parser_tx_t *tx_obj) {
  if (ctx == NULL || tx_obj == NULL) {
    return parser_unexpected_value;
  }

  CHECK_PARSER_ERR(parser_init_context(ctx, data, dataLen))
  ctx->tx_obj = tx_obj;
  if (tx_obj->tx_type == tx_textual) {
    CHECK_PARSER_ERR(_read_text_tx(ctx, tx_obj))
  } else {
    CHECK_PARSER_ERR(_read_json_tx(ctx, tx_obj))
  }

  extraDepthLevel = false;
  return parser_ok;
}

parser_error_t parser_validate(const parser_context_t *ctx) {
  if (ctx == NULL || ctx->tx_obj == NULL) {
    return parser_unexpected_value;
  }

  if (ctx->tx_obj->tx_type == tx_json) {
    CHECK_PARSER_ERR(tx_validate(&parser_tx_obj.tx_json.json))
  }

  // Iterate through all items to check that all can be shown and are valid
  uint8_t numItems = 0;
  CHECK_PARSER_ERR(parser_getNumItems(ctx, &numItems))

  char tmpKey[MAX_TITLE_SIZE];
  char tmpVal[MAX_TITLE_SIZE];
  uint8_t pageCount = 0;
  for (uint8_t idx = 0; idx < numItems; idx++) {
    CHECK_PARSER_ERR(parser_getItem(ctx, idx, tmpKey, sizeof(tmpKey), tmpVal,
                                    sizeof(tmpVal), 0, &pageCount))
  }
  return parser_ok;
}

parser_error_t parser_getNumItems(const parser_context_t *ctx,
                                  uint8_t *num_items) {
  if (ctx == NULL || ctx->tx_obj == NULL || num_items == NULL) {
    return parser_unexpected_value;
  }

  *num_items = 0;
  if (ctx->tx_obj->tx_type == tx_textual) {
    *num_items = app_mode_expert()
                     ? (uint8_t)ctx->tx_obj->tx_text.n_containers
                     : (uint8_t)(ctx->tx_obj->tx_text.n_containers -
                                 ctx->tx_obj->tx_text.n_expert);
    return parser_ok;
  }

  parser_error_t ret = tx_display_numItems(num_items);
  ctx->tx_obj->tx_json.num_items = *num_items;

  return ret;
}

__Z_INLINE bool parser_areEqual(uint16_t tokenIdx, const char *expected) {
  if (expected == NULL) {
    return false;
  }

  if (parser_tx_obj.tx_json.json.tokens[tokenIdx].type != JSMN_STRING) {
    return false;
  }

  int32_t len = parser_tx_obj.tx_json.json.tokens[tokenIdx].end -
                parser_tx_obj.tx_json.json.tokens[tokenIdx].start;
  if (len < 0) {
    return false;
  }

  if (strlen(expected) != (size_t)len) {
    return false;
  }

  const char *p = parser_tx_obj.tx_json.tx +
                  parser_tx_obj.tx_json.json.tokens[tokenIdx].start;
  for (int32_t i = 0; i < len; i++) {
    if (expected[i] != *(p + i)) {
      return false;
    }
  }

  return true;
}

__Z_INLINE bool parser_isAmount(char *key) {
  if (key == NULL) {
    return false;
  }

  if (strcmp(key, "fee/amount") == 0) {
    return true;
  }

  if (strcmp(key, "msgs/inputs/coins") == 0) {
    return true;
  }

  if (strcmp(key, "msgs/outputs/coins") == 0) {
    return true;
  }

  if (strcmp(key, "msgs/value/inputs/coins") == 0) {
    return true;
  }

  if (strcmp(key, "msgs/value/outputs/coins") == 0) {
    return true;
  }

  if (strcmp(key, "msgs/value/amount") == 0) {
    return true;
  }

  if (strcmp(key, "tip/amount") == 0) {
    return true;
  }

  return false;
}

__Z_INLINE parser_error_t is_default_denom_base(const char *denom,
                                                uint8_t denom_len,
                                                bool *is_default) {
  if (is_default == NULL) {
    return parser_unexpected_value;
  }

  bool is_expert_or_default = false;
  CHECK_PARSER_ERR(
      tx_is_expert_mode_or_not_default_chainid(&is_expert_or_default))
  if (is_expert_or_default) {
    *is_default = false;
    return parser_ok;
  }

  if (strlen(COIN_DEFAULT_DENOM_BASE) != denom_len) {
    *is_default = false;
    return parser_ok;
  }

  if (memcmp(denom, COIN_DEFAULT_DENOM_BASE, denom_len) == 0) {
    *is_default = true;
    return parser_ok;
  }

  return parser_ok;
}

__Z_INLINE parser_error_t parser_formatAmountItem(uint16_t amountToken,
                                                  char *outVal,
                                                  uint16_t outValLen,
                                                  uint8_t pageIdx,
                                                  uint8_t *pageCount) {
  if (pageCount == NULL) {
    return parser_unexpected_value;
  }

  *pageCount = 0;

  uint16_t numElements;
  CHECK_PARSER_ERR(array_get_element_count(&parser_tx_obj.tx_json.json,
                                           amountToken, &numElements))

  if (numElements == 0) {
    *pageCount = 1;
    snprintf(outVal, outValLen, "Empty");
    return parser_ok;
  }

  if (numElements != AMOUNT_OBJECT_TOKEN_COUNT) {
    return parser_unexpected_field;
  }

  if (parser_tx_obj.tx_json.json.tokens[amountToken].type != JSMN_OBJECT) {
    return parser_unexpected_field;
  }

  if (!parser_areEqual(amountToken + AMOUNT_KEY_TOKEN_OFFSET, "amount")) {
    return parser_unexpected_field;
  }

  if (!parser_areEqual(amountToken + DENOM_KEY_TOKEN_OFFSET, "denom")) {
    return parser_unexpected_field;
  }

  char bufferUI[FORMATTED_AMOUNT_BUFFER_SIZE];
  char tmpDenom[COIN_DENOM_MAXSIZE];
  char tmpAmount[COIN_AMOUNT_MAXSIZE];
  MEMZERO(tmpDenom, sizeof tmpDenom);
  MEMZERO(tmpAmount, sizeof(tmpAmount));
  MEMZERO(outVal, outValLen);
  MEMZERO(bufferUI, sizeof(bufferUI));

  if (parser_tx_obj.tx_json.json.tokens[amountToken + AMOUNT_VALUE_TOKEN_OFFSET]
              .start < 0 ||
      parser_tx_obj.tx_json.json.tokens[amountToken + DENOM_VALUE_TOKEN_OFFSET]
              .start < 0) {
    return parser_unexpected_buffer_end;
  }
  const char *amountPtr =
      parser_tx_obj.tx_json.tx +
      parser_tx_obj.tx_json.json.tokens[amountToken + AMOUNT_VALUE_TOKEN_OFFSET]
          .start;

  const int32_t amountLen =
      parser_tx_obj.tx_json.json.tokens[amountToken + AMOUNT_VALUE_TOKEN_OFFSET]
          .end -
      parser_tx_obj.tx_json.json.tokens[amountToken + AMOUNT_VALUE_TOKEN_OFFSET]
          .start;
  const char *denomPtr =
      parser_tx_obj.tx_json.tx +
      parser_tx_obj.tx_json.json.tokens[amountToken + DENOM_VALUE_TOKEN_OFFSET]
          .start;
  const int32_t denomLen =
      parser_tx_obj.tx_json.json.tokens[amountToken + DENOM_VALUE_TOKEN_OFFSET]
          .end -
      parser_tx_obj.tx_json.json.tokens[amountToken + DENOM_VALUE_TOKEN_OFFSET]
          .start;

  if (denomLen <= 0 || denomLen >= COIN_DENOM_MAXSIZE) {
    return parser_unexpected_error;
  }
  if (amountLen <= 0 || amountLen >= COIN_AMOUNT_MAXSIZE) {
    return parser_unexpected_error;
  }

  const size_t totalLen = amountLen + denomLen + 2;
  if (sizeof(bufferUI) < totalLen) {
    return parser_unexpected_buffer_end;
  }

  // Extract amount and denomination
  MEMCPY(tmpDenom, denomPtr, denomLen);
  MEMCPY(tmpAmount, amountPtr, amountLen);

  snprintf(bufferUI, sizeof(bufferUI), "%s ", tmpAmount);
  // If denomination has been recognized format and replace
  bool is_default = false;
  CHECK_PARSER_ERR(is_default_denom_base(denomPtr, denomLen, &is_default))
  if (is_default) {
    if (fpstr_to_str(bufferUI, sizeof(bufferUI), tmpAmount,
                     COIN_DEFAULT_DENOM_FACTOR) != 0) {
      return parser_unexpected_error;
    }
    number_inplace_trimming(bufferUI, COIN_DEFAULT_DENOM_TRIMMING);
    snprintf(tmpDenom, sizeof(tmpDenom), " %s", COIN_DEFAULT_DENOM_REPR);
  }

  z_str3join(bufferUI, sizeof(bufferUI), "", tmpDenom);
  pageString(outVal, outValLen, bufferUI, pageIdx, pageCount);

  return parser_ok;
}

__Z_INLINE parser_error_t parser_formatAmount(uint16_t amountToken,
                                              char *outVal, uint16_t outValLen,
                                              uint8_t pageIdx,
                                              uint8_t *pageCount) {
  if (pageCount == NULL) {
    return parser_unexpected_value;
  }

  ZEMU_LOGF(200, "[formatAmount] ------- pageidx %d", pageIdx)

  *pageCount = 0;
  if (parser_tx_obj.tx_json.json.tokens[amountToken].type != JSMN_ARRAY) {
    return parser_formatAmountItem(amountToken, outVal, outValLen, pageIdx,
                                   pageCount);
  }

  uint8_t totalPages = 0;
  uint8_t showItemSet = 0;
  uint8_t showPageIdx = pageIdx;
  uint16_t showItemTokenIdx = 0;

  uint16_t numberAmounts;
  CHECK_PARSER_ERR(array_get_element_count(&parser_tx_obj.tx_json.json,
                                           amountToken, &numberAmounts))

  // Count total subpagesCount and calculate correct page and TokenIdx
  for (uint16_t i = 0; i < numberAmounts; i++) {
    uint16_t itemTokenIdx;
    uint8_t subpagesCount;

    CHECK_PARSER_ERR(array_get_nth_element(&parser_tx_obj.tx_json.json,
                                           amountToken, i, &itemTokenIdx));
    CHECK_PARSER_ERR(parser_formatAmountItem(itemTokenIdx, outVal, outValLen, 0,
                                             &subpagesCount));

    // Check for overflow before accumulating pages
    if (totalPages > UINT8_MAX - subpagesCount) {
      return parser_value_out_of_range;
    }
    totalPages += subpagesCount;

    ZEMU_LOGF(
        200,
        "[formatAmount] [%d] TokenIdx: %d - PageIdx: %d - Pages: %d - Total %d",
        i, itemTokenIdx, showPageIdx, subpagesCount, totalPages)

    if (!showItemSet) {
      if (showPageIdx < subpagesCount) {
        showItemSet = 1;
        showItemTokenIdx = itemTokenIdx;
        ZEMU_LOGF(200, "[formatAmount] [%d] [SET] TokenIdx %d - PageIdx: %d", i,
                  showItemTokenIdx, showPageIdx)
      } else {
        if (showPageIdx < subpagesCount) {
          return parser_unexpected_value;
        }
        showPageIdx -= subpagesCount;
      }
    }
  }
  *pageCount = totalPages;
  if (pageIdx > totalPages) {
    return parser_unexpected_value;
  }

  if (totalPages == 0) {
    *pageCount = 1;
    snprintf(outVal, outValLen, "Empty");
    return parser_ok;
  }

  uint8_t dummy;
  return parser_formatAmountItem(showItemTokenIdx, outVal, outValLen,
                                 showPageIdx, &dummy);
}

#if defined(COMPILE_TEXTUAL)
__Z_INLINE parser_error_t parser_screenPrint(const parser_context_t *ctx,
                                             Cbor_container *container,
                                             char *outKey, uint16_t outKeyLen,
                                             char *outVal, uint16_t outValLen,
                                             uint8_t pageIdx,
                                             uint8_t *pageCount) {
  if (ctx == NULL || ctx->tx_obj == NULL || container == NULL ||
      pageCount == NULL) {
    return parser_unexpected_value;
  }

  // verification assures that content + title < size(tmp), to be used in string
  // manipulation
  if (container->screen.titleLen > MAX_TITLE_SIZE ||
      container->screen.contentLen > MAX_CONTENT_SIZE) {
    return parser_unexpected_value;
  }
  MEMZERO(ctx->tx_obj->tx_text.tmpBuffer,
          sizeof(ctx->tx_obj->tx_text.tmpBuffer));
  char *tmp = (char *)ctx->tx_obj->tx_text.tmpBuffer;
  size_t tmp_len = sizeof(ctx->tx_obj->tx_text.tmpBuffer);
  char out[OUTPUT_HANDLER_SIZE] = {0};

  // No Tittle screen
  if (container->screen.titleLen == 0) {
    MEMCPY(tmp, container->screen.contentPtr, container->screen.contentLen);
    CHECK_PARSER_ERR(tx_display_translation(out, sizeof(out), tmp,
                                            container->screen.contentLen))

    for (uint8_t i = 0; i < container->screen.indent; i++) {
      z_str3join(out, sizeof(out), SCREEN_INDENT, "");
    }

    snprintf(outKey, outKeyLen, " ");
    pageString(outVal, outValLen, out, pageIdx, pageCount);
    return parser_ok;
  }

  // Translate output, cpy to tmp to assure it ends in \0
  MEMZERO(tmp, tmp_len);
  if (container->screen.contentPtr == NULL) {
    return parser_unexpected_value;
  }
  MEMCPY(tmp, container->screen.contentPtr, container->screen.contentLen);
  CHECK_PARSER_ERR(tx_display_translation(out, sizeof(out), tmp,
                                          container->screen.contentLen))

  uint8_t titleLen = container->screen.titleLen + container->screen.indent;
  // Title needs to be truncated, so we concat title witn content
  if ((titleLen > PRINTABLE_TITLE_SIZE) ||
      (outValLen > 0 && ((strlen(out) / outValLen) >= 1 &&
                         titleLen > PRINTABLE_PAGINATED_TITLE_SIZE))) {

    char key[MAX_TITLE_SIZE + 2] = {0};
    MEMCPY(key, TITLE_TRUNCATE_REPLACE, strlen(TITLE_TRUNCATE_REPLACE));
    for (uint8_t i = 0; i < container->screen.indent; i++) {
      z_str3join(key, sizeof(key), SCREEN_INDENT, "");
    }

    MEMZERO(ctx->tx_obj->tx_text.tmpBuffer,
            sizeof(ctx->tx_obj->tx_text.tmpBuffer));
    if (container->screen.titlePtr == NULL) {
      return parser_unexpected_value;
    }
    MEMCPY(tmp, container->screen.titlePtr, container->screen.titleLen);
    MEMCPY(tmp + container->screen.titleLen, ": ", 2);
    MEMCPY(tmp + container->screen.titleLen + 2, out,
           sizeof(out) - container->screen.titleLen - 2);
    snprintf(outKey, outKeyLen, "%s", key);
    pageString(outVal, outValLen, tmp, pageIdx, pageCount);
    return parser_ok;
  }

  // Normal print case - Prepare title
  char key[MAX_TITLE_SIZE + 2] = {0};
  if (container->screen.titlePtr == NULL) {
    return parser_unexpected_value;
  }
  MEMCPY(key, container->screen.titlePtr, container->screen.titleLen);
  for (uint8_t i = 0; i < container->screen.indent; i++) {
    z_str3join(key, sizeof(key), SCREEN_INDENT, "");
  }
  snprintf(outKey, outKeyLen, "%s", key);
  pageString(outVal, outValLen, out, pageIdx, pageCount);

  return parser_ok;
}

__Z_INLINE parser_error_t parser_getScreenInfo(const parser_context_t *ctx,
                                               Cbor_container *container,
                                               uint8_t index) {
  CborValue it;
  CborValue containerArray_ptr;
  CborValue mapStruct_ptr;
  INIT_CBOR_PARSER(ctx, mapStruct_ptr)

  PARSER_ASSERT_OR_ERROR(!cbor_value_at_end(&mapStruct_ptr),
                         parser_unexpected_buffer_end)
  PARSER_ASSERT_OR_ERROR(cbor_value_is_map(&mapStruct_ptr),
                         parser_unexpected_type)
  CHECK_CBOR_MAP_ERR(cbor_value_enter_container(&mapStruct_ptr, &it))
  CHECK_CBOR_MAP_ERR(cbor_value_advance(&it))
  CHECK_CBOR_MAP_ERR(cbor_value_enter_container(&it, &containerArray_ptr))

  for (int i = 0; i < index; i++) {
    CHECK_CBOR_MAP_ERR(cbor_value_advance(&containerArray_ptr))
  }

  CborValue data;
  CHECK_CBOR_MAP_ERR(
      cbor_value_get_map_length(&containerArray_ptr, &container->n_field))
  CHECK_CBOR_MAP_ERR(cbor_value_enter_container(&containerArray_ptr, &data))
  CHECK_PARSER_ERR(cbor_get_containerInfo(&data, container))

  return parser_ok;
}

__Z_INLINE parser_error_t parser_getNextNonExpert(const parser_context_t *ctx,
                                                  Cbor_container *container,
                                                  uint8_t displayIdx) {

  PARSER_ASSERT_OR_ERROR(displayIdx < ctx->tx_obj->tx_text.n_containers,
                         parser_unexpected_value);

  uint8_t non_expert = 0;
  for (size_t containerIdx = 0;
       containerIdx < ctx->tx_obj->tx_text.n_containers; containerIdx++) {
    parser_getScreenInfo(ctx, container, containerIdx);
    if (!container->screen.expert) {
      non_expert++;
    } else {
      continue;
    }
    if (non_expert == displayIdx + 1) {
      break;
    }
    PARSER_ASSERT_OR_ERROR(non_expert <= displayIdx, parser_unexpected_value);
  }
  return parser_ok;
}
#endif

__Z_INLINE parser_error_t
parser_getTextualItem(const parser_context_t *ctx, uint8_t displayIdx,
                      char *outKey, uint16_t outKeyLen, char *outVal,
                      uint16_t outValLen, uint8_t pageIdx, uint8_t *pageCount) {
#if !defined(COMPILE_TEXTUAL)
  UNUSED(ctx);
  UNUSED(displayIdx);
  UNUSED(outKey);
  UNUSED(outKeyLen);
  UNUSED(outVal);
  UNUSED(outValLen);
  UNUSED(pageIdx);
  UNUSED(pageCount);
  return parser_value_out_of_range;
#else
  *pageCount = 0;

  MEMZERO(outKey, outKeyLen);
  MEMZERO(outVal, outValLen);

  uint8_t numItems;
  CHECK_PARSER_ERR(parser_getNumItems(ctx, &numItems))
  PARSER_ASSERT_OR_ERROR((numItems != 0), parser_unexpected_number_items)
  PARSER_ASSERT_OR_ERROR((displayIdx < numItems),
                         parser_display_idx_out_of_range)

  CHECK_APP_CANARY()

  Cbor_container container;
  container.screen.titlePtr = NULL;
  container.screen.titleLen = 0;
  container.screen.contentPtr = NULL;
  container.screen.contentLen = 0;
  container.screen.indent = 0;
  container.screen.expert = false;
  CHECK_PARSER_ERR(parser_getScreenInfo(ctx, &container, displayIdx))

  // title and content can be Null depending on the screen for chain id they
  // cant be null
  if (container.screen.titlePtr != NULL &&
      container.screen.contentPtr != NULL) {
    if (!strncmp(container.screen.titlePtr, "Chain id",
                 container.screen.titleLen)) {
      if (!strncmp(container.screen.contentPtr, "0",
                   container.screen.contentLen) ||
          !strncmp(container.screen.contentPtr, "1",
                   container.screen.contentLen)) {
        return parser_unexpected_chain;
      }
    }
  }

  if (!app_mode_expert()) {
    CHECK_PARSER_ERR(parser_getNextNonExpert(ctx, &container, displayIdx))
  }

  CHECK_PARSER_ERR(parser_screenPrint(ctx, &container, outKey, outKeyLen,
                                      outVal, outValLen, pageIdx, pageCount))

  return parser_ok;
#endif
}

__Z_INLINE parser_error_t parser_getJsonItem(const parser_context_t *ctx,
                                             uint8_t displayIdx, char *outKey,
                                             uint16_t outKeyLen, char *outVal,
                                             uint16_t outValLen,
                                             uint8_t pageIdx,
                                             uint8_t *pageCount) {
  if (ctx == NULL || pageCount == NULL) {
    return parser_unexpected_value;
  }

  *pageCount = 0;
  char tmpKey[QUERY_KEY_BUFFER_SIZE] = {0};

  MEMZERO(outKey, outKeyLen);
  MEMZERO(outVal, outValLen);

  uint8_t numItems;
  CHECK_PARSER_ERR(parser_getNumItems(ctx, &numItems))
  CHECK_APP_CANARY()

  if (numItems == 0) {
    return parser_unexpected_number_items;
  }

  if (displayIdx >= numItems) {
    return parser_display_idx_out_of_range;
  }

  uint16_t ret_value_token_index = 0;
  CHECK_PARSER_ERR(tx_display_query(displayIdx, tmpKey, sizeof(tmpKey),
                                    &ret_value_token_index))
  CHECK_APP_CANARY()
  snprintf(outKey, outKeyLen, "%s", tmpKey);

  if (parser_isAmount(tmpKey)) {
    CHECK_PARSER_ERR(parser_formatAmount(ret_value_token_index, outVal,
                                         outValLen, pageIdx, pageCount))
  } else {
    CHECK_PARSER_ERR(tx_getToken(ret_value_token_index, outVal, outValLen,
                                 pageIdx, pageCount))
  }
  CHECK_APP_CANARY()

  CHECK_PARSER_ERR(tx_display_make_friendly())
  CHECK_APP_CANARY()

  snprintf(outKey, outKeyLen, "%s", tmpKey);
  CHECK_APP_CANARY()

  return parser_ok;
}

parser_error_t parser_getItem(const parser_context_t *ctx, uint8_t displayIdx,
                              char *outKey, uint16_t outKeyLen, char *outVal,
                              uint16_t outValLen, uint8_t pageIdx,
                              uint8_t *pageCount) {
  if (ctx == NULL || ctx->tx_obj == NULL || pageCount == NULL) {
    return parser_unexpected_value;
  }

  if (ctx->tx_obj->tx_type == tx_textual) {
    CHECK_PARSER_ERR(parser_getTextualItem(ctx, displayIdx, outKey, outKeyLen,
                                           outVal, outValLen, pageIdx,
                                           pageCount));

  } else {
    CHECK_PARSER_ERR(parser_getJsonItem(ctx, displayIdx, outKey, outKeyLen,
                                        outVal, outValLen, pageIdx, pageCount));
  }

  return parser_ok;
}
