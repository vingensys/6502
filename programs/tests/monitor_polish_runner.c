#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "cpu/cpu.h"
#include "mem/mem.h"
#include "peripherals/uart.h"

#define MONITOR_ROM "programs/monitor/monitor.bin"
#define ECHO_UPPER_CART "programs/carts/echo_upper.bin"

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

static void expect_output(const char* needle) {
    if (strstr(uart_get_buffer(), needle) == NULL) {
        fprintf(stderr, "UART output:\n%s\n", uart_get_buffer());
        fail(needle);
    }
}

int main(void) {
    mem_init_machine(MONITOR_ROM, ECHO_UPPER_CART);
    cpu_init();
    cpu_reset();

    enqueue_text("?\nM8000\nN\nM8000.8020\nW00052A\n");
    if (!run_until_output_contains("0005=2A OK", 300000)) {
        fprintf(stderr, "UART output:\n%s\n", uart_get_buffer());
        fail("monitor did not print write feedback");
    }

    expect_output("CPU: NMOS6502");
    expect_output("ROM: E000-FFFF");
    expect_output("CART: 8000-BFFF");
    expect_output("Mhhhh    dump 16 bytes from address");
    expect_output("Mhhhh.kkkk dump address range");
    expect_output("N        dump next 16 bytes");
    expect_output("8000:");
    expect_output("8010:");
    expect_output("8020:");

    if (mem_read(0x0005) != 0x2A) {
        fail("write command did not store $2A at $0005");
    }

    if (failures != 0) {
        fprintf(stderr, "%u monitor polish test failure(s)\n", failures);
        return 1;
    }

    printf("Monitor polish tests passed\n");
    return 0;
}
