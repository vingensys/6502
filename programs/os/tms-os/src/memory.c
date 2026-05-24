#include "tmsos.h"

unsigned char tmsos_peek(unsigned int addr) {
    return *((unsigned char*)addr);
}

void tmsos_poke(unsigned int addr, unsigned char value) {
    *((unsigned char*)addr) = value;
}
