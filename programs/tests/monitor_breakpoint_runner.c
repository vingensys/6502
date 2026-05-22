#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu/cpu.h"
#include "mem/mem.h"
#include "peripherals/uart.h"

#define MONITOR_ROM "programs/monitor/monitor.bin"
#define BREAKPOINT_CART "programs/carts/breakpoint_test.bin"

uint8_t DEBUG = 0;

static unsigned failures = 0;

static void fail(const char* message) {
    fprintf(stderr, "[FAIL] %s\n", message);
    failures++;
}

static void enqueue_text(const char* text) {
    while (*text != '\0') {
        uart_enqueue_input((uint8_t)*text);
        text++;
    }
}

static uint8_t run_until_output_contains(const char* needle,
                                         unsigned max_instructions) {
    for (unsigned i = 0; i < max_instructions; i++) {
        cpu_exec();
        if (strstr(uart_get_buffer(), needle) != NULL) return 1;
    }

    return 0;
}

int main(void) {
    mem_init_machine(MONITOR_ROM, BREAKPOINT_CART);
    cpu_init();
    cpu_reset();

    enqueue_text("B8005\nC\nL\nB8005\nL\nG8000\n");
    if (!run_until_output_contains("BRK 8005", 200000)) {
        fail("monitor did not report BRK 8005");
        fprintf(stderr, "UART output:\n%s\n", uart_get_buffer());
    }

    if (strstr(uart_get_buffer(), "B8005") == NULL) {
        fail("monitor did not list active breakpoint");
    }

    if (strstr(uart_get_buffer(), "NONE") == NULL) {
        fail("monitor did not report NONE after clearing breakpoint");
    }

    if (mem_read(0x8005) != 0xEA) {
        fail("breakpoint original byte was not restored");
    }

    if (failures != 0) {
        fprintf(stderr, "%u monitor breakpoint test failure(s)\n", failures);
        return 1;
    }

    printf("Monitor breakpoint tests passed\n");
    return 0;
}
