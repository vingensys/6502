#include "kinput.h"

#include <ctype.h>
#include <ncurses.h>
#include <stdint.h>

#include "../cpu/cpu.h"
#include "interface.h"
#include "uart.h"

uint8_t QUIT = 0;
static uint8_t RUNNING = 0;

static void handle_debugger_key(int c) {
    switch (c) {
        case ERR:
            break;

        case '\n':
        case KEY_ENTER:
            cpu_exec();
            RUNNING = 0;
            break;

        case '\t':
            interface_next_mem_view();
            break;

        case KEY_BTAB:
            interface_prev_mem_view();
            break;

        case KEY_UP:
            interface_scroll_mem_view(-1);
            break;

        case KEY_DOWN:
            interface_scroll_mem_view(1);
            break;

        case KEY_PPAGE:
            interface_scroll_mem_view(-8);
            break;

        case KEY_NPAGE:
            interface_scroll_mem_view(8);
            break;

        case KEY_HOME:
            interface_mem_view_home();
            break;

        case KEY_END:
            interface_mem_view_end();
            break;

        case KEY_RESIZE:
            interface_handle_resize();
            break;

        case KEY_F(2):
            interface_set_mode(UART_TERMINAL_MODE);
            RUNNING = 1;
            break;

        case ' ':
            RUNNING = !RUNNING;
            break;

        case 'n':
            for (uint8_t i = 0; i < 10; i++) {
                cpu_exec();
            }
            RUNNING = 0;
            break;

        case 'c':
            uart_clear();
            break;

        case 'r':
            cpu_reset();
            RUNNING = 0;
            break;

        case 'q':
            QUIT = 1;
            RUNNING = 0;
            break;

        default:
            break;
    }
}

static void handle_uart_terminal_key(int c) {
    switch (c) {
        case ERR:
            break;

        case KEY_RESIZE:
            interface_handle_resize();
            break;

        case KEY_F(1):
        case 27:
        case 3:
            interface_set_mode(DEBUGGER_MODE);
            break;

        case 17:
        case 24:
            QUIT = 1;
            RUNNING = 0;
            break;

        case 18:
            cpu_reset();
            RUNNING = 1;
            break;

        case '\n':
        case KEY_ENTER:
            uart_enqueue_input('\n');
            break;

        case KEY_BACKSPACE:
        case 127:
        case '\b':
            uart_enqueue_input(0x08);
            break;

        default:
            if (c >= 0 && c <= UINT8_MAX && isprint(c)) {
                uart_enqueue_input((uint8_t)c);
            }
            break;
    }
}

/**
 * kinput_listen: listens for keyboard events and exuctes respective actions
 * @param void
 * @return void
 * */
void kinput_listen(void) {
    int c = getch();

    if (interface_get_mode() == UART_TERMINAL_MODE) {
        handle_uart_terminal_key(c);
    } else {
        handle_debugger_key(c);
    }
}

// kinput_should_quit: sends quit signal by returning QUIT status
uint8_t kinput_should_quit(void) { return QUIT; }

uint8_t kinput_is_running(void) { return RUNNING; }

void kinput_set_running(uint8_t running) { RUNNING = running ? 1 : 0; }
