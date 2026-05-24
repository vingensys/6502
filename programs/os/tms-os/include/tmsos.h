#ifndef TMSOS_H
#define TMSOS_H

#define TMSOS_VERSION "0.1"

void __fastcall__ os_putc(unsigned char value);
unsigned char os_getc(void);
void __fastcall__ os_puts(const char* text);
void os_newline(void);
void os_warm_start(void);

void kernel_main(void);
void kernel_warm_start(void);
void shell_run(void);
void shell_execute(const char* line);

unsigned char streq_ci(const char* left, const char* right);

#endif
