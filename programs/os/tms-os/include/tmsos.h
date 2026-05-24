#ifndef TMSOS_H
#define TMSOS_H

#define TMSOS_VERSION "0.1"

#define TMSOS_OS_WARM_START 0xE100
#define TMSOS_OS_PUTC 0xE103
#define TMSOS_OS_GETC 0xE106
#define TMSOS_OS_PUTS 0xE109
#define TMSOS_OS_NEWLINE 0xE10C

void __fastcall__ os_putc(unsigned char value);
unsigned char os_getc(void);
void __fastcall__ os_puts(const char* text);
void os_newline(void);
void os_warm_start(void);
void __fastcall__ os_run(unsigned int addr);

void kernel_main(void);
void kernel_warm_start(void);
void shell_run(void);
void shell_execute(const char* line);

unsigned char tmsos_peek(unsigned int addr);
void tmsos_poke(unsigned int addr, unsigned char value);

unsigned char streq_ci(const char* left, const char* right);
unsigned char parse_hex8(const char* text, unsigned char* value);
unsigned char parse_hex16(const char* text, unsigned int* value);
void os_put_hex8(unsigned char value);
void os_put_hex16(unsigned int value);

#endif
