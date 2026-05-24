#include "tmsos.h"

#define LINE_SIZE 32

static char line[LINE_SIZE + 1];

static void read_line(void) {
    unsigned char len = 0;

    while (1) {
        unsigned char ch = os_getc();

        if (ch == '\r' || ch == '\n') {
            line[len] = '\0';
            os_newline();
            return;
        }

        if (ch == 0x08 || ch == 0x7F) {
            if (len > 0) {
                len--;
            }
            continue;
        }

        if (len < LINE_SIZE) {
            line[len++] = (char)ch;
            os_putc(ch);
        }
    }
}

static void cmd_help(void) {
    os_puts("HELP INFO RESET\n");
}

static void cmd_info(void) {
    os_puts("CPU NMOS6502\n");
    os_puts("ROM E000-FFFF\n");
    os_puts("CART 8000-BFFF\n");
    os_puts("UART D010-D011\n");
}

void shell_execute(const char* command) {
    if (command[0] == '\0') {
        return;
    }
    if (streq_ci(command, "HELP")) {
        cmd_help();
    } else if (streq_ci(command, "INFO")) {
        cmd_info();
    } else if (streq_ci(command, "RESET")) {
        os_warm_start();
    } else {
        os_puts("ERR\n");
    }
}

void shell_run(void) {
    while (1) {
        os_putc('>');
        read_line();
        shell_execute(line);
    }
}
