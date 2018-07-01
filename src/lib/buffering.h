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

#ifndef CI_TEST_BUFFERING_H
#define CI_TEST_BUFFERING_H

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t *data;
    uint16_t size;
    uint16_t pos;
    uint8_t in_use: 1;
    uint8_t initialized : 1;
} buffer_state_t;

typedef void (*append_buffer_delegate)(buffer_state_t *buffer, uint8_t *data, int size);

void buffering_init(
        uint8_t *ram_buffer,
        uint16_t ram_buffer_size,
        append_buffer_delegate ram,
        uint8_t *flash_buffer,
        uint16_t flash_buffer_size,
        append_buffer_delegate flash);

void buffering_reset();

void buffering_append(uint8_t *data, int length);

buffer_state_t *buffering_get_ram_buffer();
buffer_state_t *buffering_get_flash_buffer();
buffer_state_t *buffering_get_buffer();


#ifdef __cplusplus
}
#endif

#endif //CI_TEST_BUFFERING_H