#ifndef INC_6502_UART_H
#define INC_6502_UART_H

#include <stddef.h>
#include <stdint.h>

void uart_init(void);
void uart_write_data(uint8_t value);
uint8_t uart_read_data(void);
uint8_t uart_read_status(void);
const char* uart_get_buffer(void);
size_t uart_get_buffer_len(void);
void uart_clear(void);

#endif
