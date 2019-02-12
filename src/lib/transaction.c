/*******************************************************************************
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

#include "transaction.h"
#include "../view.h"
#include "apdu_codes.h"
#include "json_parser.h"
#include "buffering.h"

// Ram
#define RAM_BUFFER_SIZE 448
uint8_t ram_buffer[RAM_BUFFER_SIZE];

// Flash
#define FLASH_BUFFER_SIZE 10000
typedef struct {
    uint8_t buffer[FLASH_BUFFER_SIZE];
} storage_t;

storage_t N_appdata_impl __attribute__ ((aligned(64)));
#define N_appdata (*(storage_t *)PIC(&N_appdata_impl))

parsed_json_t parsed_transaction;

void update_ram(buffer_state_t *buffer, uint8_t *data, int size) {
    os_memmove(buffer->data + buffer->pos, data, size);
}

void update_flash(buffer_state_t *buffer, uint8_t *data, int size) {
    nvm_write((void *) buffer->data + buffer->pos, data, size);
}

void transaction_initialize() {
    append_buffer_delegate update_ram_delegate = &update_ram;
    append_buffer_delegate update_flash_delegate = &update_flash;

    buffering_init(
        ram_buffer,
        sizeof(ram_buffer),
        update_ram_delegate,
        N_appdata.buffer,
        sizeof(N_appdata.buffer),
        update_flash_delegate
    );
}

void transaction_reset() {
    buffering_reset();
}

uint32_t transaction_append(unsigned char *buffer, uint32_t length) {
    return buffering_append(buffer, length);
}

uint32_t transaction_get_buffer_length() {
    return buffering_get_buffer()->pos;
}

uint8_t *transaction_get_buffer() {
    return buffering_get_buffer()->data;
}

const char* transaction_parse() {
    const char *transaction_buffer = (const char *) transaction_get_buffer();
    const char* error_msg = json_parse_s(&parsed_transaction, transaction_buffer, transaction_get_buffer_length());
    if (error_msg != NULL) {
        return error_msg;
    }
    error_msg = json_validate(&parsed_transaction, transaction_buffer);
    if (error_msg != NULL) {
        return error_msg;
    }
    parsing_context_t context;
    context.transaction = transaction_buffer;
    context.max_chars_per_key_line = MAX_CHARS_PER_KEY_LINE;
    context.max_chars_per_value_line = MAX_CHARS_PER_VALUE_LINE;
    context.parsed_transaction = &parsed_transaction;
    set_parsing_context(context);
    set_copy_delegate(&os_memmove);
    return NULL;
}

parsed_json_t *transaction_get_parsed() {
    return &parsed_transaction;
}
