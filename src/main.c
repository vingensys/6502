#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>

#include "cpu/cpu.h"
#include "mem/mem.h"
#include "peripherals/interface.h"
#include "peripherals/kinput.h"

uint8_t DEBUG = 0;

static void draw_interface(void) {
    interface_display_header();
    interface_display_cpu();
    interface_display_mem();
}

int main(int argc, char** argv) {
    switch (argc) {
        case 1:
            mem_init("example.bin");
            break;

        case 2:
            mem_init(argv[1]);
            break;

        default:
            fprintf(stderr, "[FAILED] Too many arguments.\n");
            exit(1);
    }

    cpu_init();
    cpu_reset();

    WINDOW* win;
    if ((win = initscr()) == NULL) {
        fprintf(stderr, "[FAILED] Error initialising ncurses.\n");
        exit(1);
    }
    curs_set(0);
    noecho();
    keypad(stdscr, TRUE);

    timeout(0);
    draw_interface();

    do {
        timeout(kinput_is_running() ? 0 : -1);
        draw_interface();

        kinput_listen();
        if (kinput_is_running()) {
            cpu_exec();
            napms(30);
        }
    } while (!kinput_should_quit());

    (void)win;
    interface_shutdown();
    endwin();

    mem_dump();
    return 0;
}
