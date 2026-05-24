#ifndef TMSOS_CART_H
#define TMSOS_CART_H

#define TMSOS_OS_WARM_START 0xE100
#define TMSOS_OS_PUTC 0xE103
#define TMSOS_OS_GETC 0xE106
#define TMSOS_OS_PUTS 0xE109
#define TMSOS_OS_NEWLINE 0xE10C

void __fastcall__ os_putc(char c);
char os_getc(void);
void __fastcall__ os_puts(const char* s);
void os_newline(void);
void os_warm_start(void);

#endif
