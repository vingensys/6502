#include "tmsos_cart.h"

#define MAX_NAME 24

static void read_line(char *buf, unsigned char max)
{
    unsigned char i = 0;
    char c;

    while (1) {
        c = os_getc();

        if (c == '\r' || c == '\n') {
            os_newline();
            break;
        }

        if (i < max - 1) {
            buf[i++] = c;
            os_putc(c);   /* echo */
        }
    }

    buf[i] = '\0';
}

int main(void)
{
    char name[MAX_NAME];

    os_puts("TMS-GREETER 0.1");
    os_newline();

    os_puts("WHAT IS YOUR NAME?");
    os_newline();

    os_puts("> ");
    read_line(name, MAX_NAME);

    os_puts("HELLO, ");
    os_puts(name);
    os_newline();

    os_puts("RETURNING TO TMS-OS...");
    os_newline();

    os_warm_start();

    return 0;
}