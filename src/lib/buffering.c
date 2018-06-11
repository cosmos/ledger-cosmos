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

#include "buffering.h"

// Ram
append_buffer_delegate append_ram_buffer = NULL;
buffer_state_t ram;

// Move to a separate file
#define RAM_BUFFER_SIZE 512
uint8_t ram_buffer[RAM_BUFFER_SIZE];

// Flash
append_buffer_delegate append_flash_buffer = NULL;
buffer_state_t flash;

// Move to a separate file
#define FLASH_BUFFER_SIZE 16384
typedef struct {
    uint8_t buffer[FLASH_BUFFER_SIZE];
} storage_t;
extern storage_t N_appdata_impl;
#define N_appdata (*(storage_t *)PIC(&N_appdata_impl))


void buffering_init(
        uint8_t* ram_buffer,
        int ram_buffer_size,
        append_buffer_delegate ram_delegate,
        uint8_t* flash_buffer,
        int flash_buffer_size,
        append_buffer_delegate flash_delegate)
{
    append_ram_buffer = ram_delegate;
    append_flash_buffer = flash_delegate;

    ram.data = ram_buffer;
    ram.size = ram_buffer_size;
    ram.pos = 0;
    ram.in_use = 1;
    ram.initialized = 1;

    flash.data = flash_buffer;
    flash.size = flash_buffer_size;
    flash.pos = 0;
    flash.in_use = 0;
    flash.initialized = 1;
}

void buffering_reset()
{
    ram.pos = 0;
    ram.in_use = 1;
    flash.pos = 0;
    flash.in_use = 0;
}

void buffering_append(uint8_t* data, int length)
{
    if (ram.in_use) {
        if (ram.size - ram.pos >= length) {
            append_ram_buffer(&ram, data, length);
        }
        else {
            ram.in_use = 0;
            flash.in_use = 1;
            buffering_append(ram.data, ram.pos);
            buffering_append(data,length);
            ram.pos = 0;
        }
    }
    else {
        append_flash_buffer(&flash, data, length);
    }
}


buffer_state_t* buffering_get_ram_buffer()
{
    return &ram;
}

buffer_state_t* buffering_get_flash_buffer()
{
    return &flash;
}