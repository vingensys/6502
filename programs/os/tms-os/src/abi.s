        .export OS_WARM_START
        .export OS_PUTC
        .export OS_GETC
        .export OS_PUTS
        .export OS_NEWLINE
        .import _os_warm_start
        .import _os_putc
        .import _os_getc
        .import _os_puts
        .import _os_newline

USER_STR_PTR = $80

        .segment "ZEROPAGE"
abi_save_x: .res 1
abi_save_y: .res 1

        .segment "ABI"

OS_WARM_START:
        jmp _os_warm_start
OS_PUTC:
        jmp abi_putc
OS_GETC:
        jmp abi_getc
OS_PUTS:
        jmp abi_puts
OS_NEWLINE:
        jmp abi_newline

        .segment "CODE"

abi_putc:
        stx abi_save_x
        sty abi_save_y
        jsr _os_putc
        ldx abi_save_x
        ldy abi_save_y
        rts

abi_getc:
        stx abi_save_x
        sty abi_save_y
        jsr _os_getc
        ldx abi_save_x
        ldy abi_save_y
        rts

abi_puts:
        stx abi_save_x
        sty abi_save_y
        lda USER_STR_PTR
        ldx USER_STR_PTR+1
        jsr _os_puts
        ldx abi_save_x
        ldy abi_save_y
        rts

abi_newline:
        stx abi_save_x
        sty abi_save_y
        jsr _os_newline
        ldx abi_save_x
        ldy abi_save_y
        rts
