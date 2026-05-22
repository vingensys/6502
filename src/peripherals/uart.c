#include "uart.h"

#include <ctype.h>
#include <string.h>

#define UART_BUFFER_SIZE 4096
#define UART_STATUS_TX_READY 0x01

static char terminal_buffer[UART_BUFFER_SIZE];
static size_t terminal_len = 0;

void uart_init(void) {
    uart_clear();
}

void uart_write_data(uint8_t value) {
    char ch;

    if (value == '\n') {
        ch = '\n';
    } else if (isprint(value)) {
        ch = (char)value;
    } else {
        ch = '.';
    }

    if (terminal_len >= UART_BUFFER_SIZE - 1) {
        memmove(terminal_buffer, terminal_buffer + 1, UART_BUFFER_SIZE - 2);
        terminal_len = UART_BUFFER_SIZE - 2;
    }

    terminal_buffer[terminal_len++] = ch;
    terminal_buffer[terminal_len] = '\0';
}

uint8_t uart_read_data(void) {
    return 0;
}

uint8_t uart_read_status(void) {
    return UART_STATUS_TX_READY;
}

const char* uart_get_buffer(void) {
    return terminal_buffer;
}

size_t uart_get_buffer_len(void) {
    return terminal_len;
}

void uart_clear(void) {
    terminal_len = 0;
    terminal_buffer[0] = '\0';
}
