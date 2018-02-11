#include <os_io_seproxyhal.h>

__attribute__((section(".boot"))) int
main(void)
{
    // exit critical section
    __asm volatile("cpsie i");

    ui_init();
    os_boot();

    UX_INIT();

    BEGIN_TRY
    {
        TRY
        {
            io_seproxyhal_init();
            app_init();

            USB_power(0);
            USB_power(1);

            app_main();
        }
        CATCH_OTHER(e)
        {}
        FINALLY
        {}
    }
    END_TRY;
}
