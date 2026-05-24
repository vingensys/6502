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
