#include "tmsos.h"

static char upper(char ch) {
    if (ch >= 'a' && ch <= 'z') {
        return (char)(ch - ('a' - 'A'));
    }
    return ch;
}

unsigned char streq_ci(const char* left, const char* right) {
    while (*left != '\0' && *right != '\0') {
        if (upper(*left) != upper(*right)) {
            return 0;
        }
        left++;
        right++;
    }

    return *left == '\0' && *right == '\0';
}

static unsigned char hex_value(char ch, unsigned char* value) {
    if (ch >= '0' && ch <= '9') {
        *value = (unsigned char)(ch - '0');
        return 1;
    }
    ch = upper(ch);
    if (ch >= 'A' && ch <= 'F') {
        *value = (unsigned char)(ch - 'A' + 10);
        return 1;
    }
    return 0;
}

static char hex_digit(unsigned char value) {
    value &= 0x0F;
    if (value < 10) {
        return (char)('0' + value);
    }
    return (char)('A' + value - 10);
}

unsigned char parse_hex8(const char* text, unsigned char* value) {
    unsigned char hi;
    unsigned char lo;

    if (!hex_value(text[0], &hi) || !hex_value(text[1], &lo)) {
        return 0;
    }
    if (text[2] != '\0' && text[2] != ' ' && text[2] != '\t') {
        return 0;
    }

    *value = (unsigned char)((hi << 4) | lo);
    return 1;
}

unsigned char parse_hex16(const char* text, unsigned int* value) {
    unsigned char n0;
    unsigned char n1;
    unsigned char n2;
    unsigned char n3;

    if (!hex_value(text[0], &n0) || !hex_value(text[1], &n1) ||
        !hex_value(text[2], &n2) || !hex_value(text[3], &n3)) {
        return 0;
    }
    if (text[4] != '\0' && text[4] != ' ' && text[4] != '\t') {
        return 0;
    }

    *value = (unsigned int)((n0 << 12) | (n1 << 8) | (n2 << 4) | n3);
    return 1;
}

void os_put_hex8(unsigned char value) {
    os_putc((unsigned char)hex_digit((unsigned char)(value >> 4)));
    os_putc((unsigned char)hex_digit(value));
}

void os_put_hex16(unsigned int value) {
    os_put_hex8((unsigned char)(value >> 8));
    os_put_hex8((unsigned char)value);
}
