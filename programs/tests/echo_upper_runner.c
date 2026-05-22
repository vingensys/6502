#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "cpu/cpu.h"
#include "mem/mem.h"
#include "peripherals/uart.h"

#define ECHO_UPPER_CART "programs/carts/echo_upper.bin"

uint8_t DEBUG = 0;

static void enqueue_text(const char* text) {
    while (*text != '\0') {
        uart_enqueue_input((uint8_t)*text);
        text++;
    }
}

int main(void) {
    mem_init_machine(NULL, ECHO_UPPER_CART);
    cpu_init();
    cpu_reset();

    enqueue_text("hello 6502\n");
    for (unsigned i = 0; i < 200000; i++) {
        cpu_exec();
        if (strstr(uart_get_buffer(), "HELLO 6502") != NULL) {
            printf("Echo upper cartridge tests passed\n");
            return 0;
        }
    }

    fprintf(stderr, "UART output:\n%s\n", uart_get_buffer());
    fprintf(stderr, "[FAIL] echo_upper did not uppercase UART input\n");
    return 1;
}
