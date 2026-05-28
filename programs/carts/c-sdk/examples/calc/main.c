#include "tmsos_cart.h"

#define LINE_SIZE 32

static char line[LINE_SIZE + 1];

static void print_str(const char* text) {
    os_puts(text);
}

static char upper(char ch) {
    if (ch >= 'a' && ch <= 'z') {
        return (char)(ch - ('a' - 'A'));
    }
    return ch;
}

static char is_space(char ch) {
    return ch == ' ' || ch == '\t';
}

static const char* skip_spaces(const char* text) {
    while (is_space(*text)) {
        text++;
    }
    return text;
}

static void read_line(char* buf, unsigned char max) {
    unsigned char len = 0;

    while (1) {
        char ch = os_getc();

        if (ch == '\r' || ch == '\n') {
            buf[len] = '\0';
            os_newline();
            return;
        }

        if (ch == 0x08 || ch == 0x7F) {
            if (len > 0) {
                len--;
                os_putc(0x08);
                os_putc(' ');
                os_putc(0x08);
            }
            continue;
        }

        if (len < max) {
            buf[len++] = ch;
            os_putc(ch);
        }
    }
}

static unsigned char parse_uint(const char** text, unsigned int* value) {
    unsigned int result = 0;
    unsigned char digits = 0;
    const char* p = skip_spaces(*text);

    while (*p >= '0' && *p <= '9') {
        result = (unsigned int)((result * 10U) + (unsigned int)(*p - '0'));
        p++;
        digits++;
    }

    if (digits == 0) {
        return 0;
    }

    *value = result;
    *text = p;
    return 1;
}

static void print_uint(unsigned int value) {
    char digits[5];
    unsigned char count = 0;

    if (value == 0) {
        os_putc('0');
        return;
    }

    while (value > 0 && count < sizeof(digits)) {
        digits[count++] = (char)('0' + (value % 10U));
        value = (unsigned int)(value / 10U);
    }

    while (count > 0) {
        os_putc(digits[--count]);
    }
}

static void print_result(unsigned int value) {
    print_str("= ");
    print_uint(value);
    os_newline();
}

static void eval_line(const char* text) {
    unsigned int left;
    unsigned int right;
    unsigned int result;
    char op;

    text = skip_spaces(text);
    if ((upper(text[0]) == 'Q') && (skip_spaces(text + 1)[0] == '\0')) {
        os_warm_start();
    }

    if (!parse_uint(&text, &left)) {
        print_str("ERR\n");
        return;
    }

    text = skip_spaces(text);
    op = *text++;
    if (op != '+' && op != '-' && op != '*' && op != '/') {
        print_str("ERR\n");
        return;
    }

    if (!parse_uint(&text, &right)) {
        print_str("ERR\n");
        return;
    }

    if (skip_spaces(text)[0] != '\0') {
        print_str("ERR\n");
        return;
    }

    if (op == '+') {
        result = (unsigned int)(left + right);
    } else if (op == '-') {
        if (right > left) {
            print_str("ERR\n");
            return;
        }
        result = (unsigned int)(left - right);
    } else if (op == '*') {
        result = (unsigned int)(left * right);
    } else {
        if (right == 0) {
            print_str("DIV ZERO\n");
            return;
        }
        result = (unsigned int)(left / right);
    }

    print_result(result);
}

int main(void) {
    print_str("TMS-CALC 0.1\n");
    print_str("ENTER A+B, A-B, A*B, A/B\n");
    print_str("Q TO QUIT\n");

    while (1) {
        print_str("CALC>");
        read_line(line, LINE_SIZE);
        eval_line(line);
    }

    return 0;
}
