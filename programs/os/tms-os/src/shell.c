#include "tmsos.h"

#define LINE_SIZE 32

static char line[LINE_SIZE + 1];
static unsigned int last_mem_addr = 0x8000;

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
    os_puts("HELP INFO MAP MEM PEEK POKE RUN RESET\n");
}

static void cmd_info(void) {
    os_puts("CPU NMOS6502\n");
    os_puts("OS TMS-OS 0.1\n");
    os_puts("OS ZP 0000-007F\n");
    os_puts("USER ZP 0080-00FF\n");
    os_puts("STACK 0100-01FF\n");
    os_puts("OS RAM 0200-03FF\n");
    os_puts("USER RAM 0400-7FFF\n");
    os_puts("CART 8000-BFFF\n");
    os_puts("ROM E000-FFFF\n");
    os_puts("UART D010-D011\n");
}

static void cmd_map(void) {
    os_puts("OS ZP    0000-007F\n");
    os_puts("USER ZP  0080-00FF\n");
    os_puts("STACK    0100-01FF\n");
    os_puts("OS RAM   0200-03FF\n");
    os_puts("USER RAM 0400-7FFF\n");
    os_puts("CART ROM 8000-BFFF\n");
    os_puts("EXP RAM  C000-CFFF\n");
    os_puts("IO       D000-DFFF\n");
    os_puts("OS ROM   E000-FFFF\n");
}

static unsigned char is_space(char ch) {
    return ch == ' ' || ch == '\t';
}

static const char* skip_spaces(const char* text) {
    while (is_space(*text)) {
        text++;
    }
    return text;
}

static char upper_ch(char ch) {
    if (ch >= 'a' && ch <= 'z') {
        return (char)(ch - ('a' - 'A'));
    }
    return ch;
}

static unsigned char command_match(const char* text, const char* command,
                                   const char** args) {
    while (*command != '\0') {
        if (upper_ch(*text) != *command) {
            return 0;
        }
        text++;
        command++;
    }

    if (*text != '\0' && !is_space(*text)) {
        return 0;
    }

    *args = skip_spaces(text);
    return 1;
}

static unsigned char parse_addr_arg(const char* args, unsigned int* addr) {
    args = skip_spaces(args);
    return parse_hex16(args, addr);
}

static unsigned char parse_addr_byte_args(const char* args, unsigned int* addr,
                                          unsigned char* value) {
    args = skip_spaces(args);
    if (!parse_hex16(args, addr)) {
        return 0;
    }
    args = skip_spaces(args + 4);
    return parse_hex8(args, value);
}

static void put_addr_value(unsigned int addr, unsigned char value) {
    os_put_hex16(addr);
    os_puts(" = ");
    os_put_hex8(value);
}

static unsigned char can_poke(unsigned int addr) {
    if (addr >= 0x0080 && addr <= 0x00FF) {
        return 1;
    }
    if (addr >= 0x0400 && addr <= 0x7FFF) {
        return 1;
    }
    if (addr >= 0xC000 && addr <= 0xCFFF) {
        return 1;
    }
    return 0;
}

static void cmd_mem_at(unsigned int addr) {
    unsigned char i;

    os_put_hex16(addr);
    os_puts(": ");
    for (i = 0; i < 16; i++) {
        os_put_hex8(tmsos_peek((unsigned int)(addr + i)));
        if (i != 15) {
            os_putc(' ');
        }
    }
    os_newline();
    last_mem_addr = (unsigned int)(addr + 16);
}

static void cmd_peek(const char* args) {
    unsigned int addr;

    if (!parse_addr_arg(args, &addr)) {
        os_puts("ERR\n");
        return;
    }

    put_addr_value(addr, tmsos_peek(addr));
    os_newline();
}

static void cmd_poke(const char* args) {
    unsigned int addr;
    unsigned char value;

    if (!parse_addr_byte_args(args, &addr, &value)) {
        os_puts("ERR\n");
        return;
    }
    if (!can_poke(addr)) {
        os_puts("PROTECTED\n");
        return;
    }

    tmsos_poke(addr, value);
    put_addr_value(addr, value);
    os_puts(" OK\n");
}

static void cmd_mem(const char* args) {
    unsigned int addr;

    if (*args == '\0') {
        cmd_mem_at(last_mem_addr);
        return;
    }

    if (!parse_addr_arg(args, &addr)) {
        os_puts("ERR\n");
        return;
    }

    cmd_mem_at(addr);
}

static void cmd_run(const char* args) {
    unsigned int addr;

    if (!parse_addr_arg(args, &addr)) {
        os_puts("ERR\n");
        return;
    }

    os_run(addr);
}

void shell_execute(const char* command) {
    const char* args;

    command = skip_spaces(command);
    if (command[0] == '\0') {
        return;
    }

    if (streq_ci(command, "HELP") || streq_ci(command, "?")) {
        cmd_help();
    } else if (command_match(command, "INFO", &args) && *args == '\0') {
        cmd_info();
    } else if (command_match(command, "MAP", &args) && *args == '\0') {
        cmd_map();
    } else if (command_match(command, "RESET", &args) && *args == '\0') {
        os_warm_start();
    } else if (command_match(command, "MEM", &args) ||
               command_match(command, "M", &args)) {
        cmd_mem(args);
    } else if (command_match(command, "NEXT", &args) && *args == '\0') {
        cmd_mem_at(last_mem_addr);
    } else if (command_match(command, "PEEK", &args)) {
        cmd_peek(args);
    } else if (command_match(command, "POKE", &args)) {
        cmd_poke(args);
    } else if (command_match(command, "RUN", &args) ||
               command_match(command, "G", &args)) {
        cmd_run(args);
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
