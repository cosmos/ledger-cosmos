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
#include <os_io_seproxyhal.h>
#include "app_main.h"

ux_state_t ux;
unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];

void ui_init() {
    UX_INIT();
}

unsigned char io_event(unsigned char channel) {
    return 1;
}

__attribute__((section(".boot"))) int
main(void) {
    // exit critical section
    __asm volatile("cpsie i");

    ui_init();
    os_boot();

    BEGIN_TRY
    {
        TRY
        {
            app_init();
            app_main();
        }
        CATCH_OTHER(e)
        {}
        FINALLY
        {}
    }
    END_TRY;
}
