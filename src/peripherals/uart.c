#include "uart.h"

#include <ctype.h>
#include <string.h>

#define UART_BUFFER_SIZE 4096
#define UART_RX_BUFFER_SIZE 256
#define UART_STATUS_TX_READY 0x01
#define UART_STATUS_RX_READY 0x02

static char terminal_buffer[UART_BUFFER_SIZE];
static size_t terminal_len = 0;
static uint8_t rx_buffer[UART_RX_BUFFER_SIZE];
static size_t rx_head = 0;
static size_t rx_tail = 0;
static size_t rx_count = 0;

static void uart_clear_rx(void) {
    rx_head = 0;
    rx_tail = 0;
    rx_count = 0;
}

void uart_init(void) {
    uart_clear();
    uart_clear_rx();
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
    uint8_t value;

    if (rx_count == 0) return 0;

    value = rx_buffer[rx_tail];
    rx_tail = (rx_tail + 1) % UART_RX_BUFFER_SIZE;
    rx_count--;

    return value;
}

uint8_t uart_read_status(void) {
    uint8_t status = UART_STATUS_TX_READY;

    if (rx_count > 0) status |= UART_STATUS_RX_READY;

    return status;
}

void uart_enqueue_input(uint8_t value) {
    if (rx_count >= UART_RX_BUFFER_SIZE) {
        rx_tail = (rx_tail + 1) % UART_RX_BUFFER_SIZE;
        rx_count--;
    }

    rx_buffer[rx_head] = value;
    rx_head = (rx_head + 1) % UART_RX_BUFFER_SIZE;
    rx_count++;
}

const char* uart_get_buffer(void) {
    return terminal_buffer;
}

size_t uart_get_buffer_len(void) {
    return terminal_len;
}

size_t uart_get_rx_count(void) {
    return rx_count;
}

void uart_clear(void) {
    terminal_len = 0;
    terminal_buffer[0] = '\0';
}
