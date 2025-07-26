#include <stdio.h>
#include <bsp/board.h>
#include <tusb.h>
#include <pico/stdlib.h>
#include <pico/cyw43_arch.h>

int main()
{
    // Initialize TinyUSB stack
    board_init();
    tusb_init();

    // Initialize stdio (USB CDC)
    stdio_init_all();

    // Initialize Wi-Fi chip (required for Pico W)
    if (cyw43_arch_init()) {
        printf("Wi-Fi chip init failed.\n");
    }

    // Turn on LED to indicate device is running
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    printf("Pico TinyUSB MSC Bug Test - Device Ready\n");
    fflush(stdout);

    // Schedule first heartbeat one second from now
    absolute_time_t next_heartbeat = make_timeout_time_ms(1000);

    // Main loop: call tud_task() as fast as possible to trigger the bug
    while (true) {
        tud_task();

        if (time_reached(next_heartbeat)) {
            printf("heartbeat\n");
            fflush(stdout);
            // Schedule next heartbeat one second later
            next_heartbeat = delayed_by_ms(next_heartbeat, 1000);
        }
    }
}
