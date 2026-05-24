        .export _os_putc
        .export _os_getc
        .export _os_puts
        .export _os_newline
        .export _os_warm_start

OS_WARM_START = $E100
OS_PUTC       = $E103
OS_GETC       = $E106
OS_PUTS       = $E109
OS_NEWLINE    = $E10C
OS_STR_PTR    = $80

        .segment "CODE"

; void __fastcall__ os_putc(char c)
_os_putc:
        jmp OS_PUTC

; char os_getc(void)
_os_getc:
        jmp OS_GETC

; void __fastcall__ os_puts(const char* s)
_os_puts:
        sta OS_STR_PTR
        stx OS_STR_PTR+1
        jmp OS_PUTS

; void os_newline(void)
_os_newline:
        jmp OS_NEWLINE

; void os_warm_start(void)
_os_warm_start:
        jmp OS_WARM_START
