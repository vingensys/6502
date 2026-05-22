#include "kinput.h"

#include <ncurses.h>
#include <stdint.h>

#include "../cpu/cpu.h"
#include "interface.h"
#include "uart.h"

uint8_t QUIT = 0;
static uint8_t RUNNING = 0;

/**
 * kinput_listen: listens for keyboard events and exuctes respective actions
 * @param void
 * @return void
 * */
void kinput_listen(void) {
    int c = getch();

    switch (c) {
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

// kinput_should_quit: sends quit signal by returning QUIT status
uint8_t kinput_should_quit(void) { return QUIT; }

uint8_t kinput_is_running(void) { return RUNNING; }
