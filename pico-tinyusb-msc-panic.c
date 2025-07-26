#include <stdio.h>
#include <bsp/board.h>
#include <tusb.h>
#include <pico/stdlib.h>
#include <pico/cyw43_arch.h>

// TinyUSB callbacks for debugging
void tud_mount_cb(void) {
    printf("USB MSC device mounted\n");
    fflush(stdout);
}

void tud_umount_cb(void) {
    printf("USB MSC device unmounted\n");
    fflush(stdout);
}

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
        return -1;
    }

    // Turn on LED to indicate device is running
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    printf("Pico TinyUSB MSC Bug Test - Device Ready\n");
    fflush(stdout);

    // Main loop: call tud_task() as fast as possible to trigger the bug
    while (true) {
        tud_task();
    }
}
