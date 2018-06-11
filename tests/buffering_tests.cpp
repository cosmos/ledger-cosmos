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

#include "gtest/gtest.h"
#include "lib/buffering.h"

namespace {

    TEST(Buffering, SmallBuffer) {

        uint8_t ram_buffer[100];
        uint8_t flash_buffer[1000];

        buffering_init(
                ram_buffer,
                sizeof(ram_buffer),
                [](buffer_state_t* buffer, uint8_t* data, int size) {
                    memcpy(buffer->data+buffer->pos, data, size);
                    buffer->pos += size;
                },
                flash_buffer,
                sizeof(flash_buffer),
                [](buffer_state_t* buffer, uint8_t* data, int size) {
                    memcpy(buffer->data+buffer->pos, data, size);
                    buffer->pos += size;
                });

        uint8_t small[100];
        buffering_append(small, sizeof(small));
        EXPECT_TRUE(buffering_get_ram_buffer()->in_use) << "Writing small buffer should only write to RAM";
        EXPECT_FALSE(buffering_get_flash_buffer()->in_use) << "Writing big buffer should write data to FLASH";
    }

    TEST(Buffering, BigBuffer) {

        uint8_t ram_buffer[100];
        uint8_t flash_buffer[1000];

        buffering_init(
                ram_buffer,
                sizeof(ram_buffer),
                [](buffer_state_t* buffer, uint8_t* data, int size) {
                    memcpy(buffer->data+buffer->pos, data, size);
                    buffer->pos += size;
                },
                flash_buffer,
                sizeof(flash_buffer),
                [](buffer_state_t* buffer, uint8_t* data, int size) {
                    memcpy(buffer->data+buffer->pos, data, size);
                    buffer->pos += size;
                });

        uint8_t big[1000];
        buffering_append(big, sizeof(big));
        EXPECT_FALSE(buffering_get_ram_buffer()->in_use) << "Writing big buffer should write data to FLASH";
        EXPECT_TRUE(buffering_get_flash_buffer()->in_use) << "Writing big buffer should write data to FLASH";
    }
}