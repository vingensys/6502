        .export _os_putc
        .export _os_getc
        .export _os_puts
        .export _os_newline
        .export _os_warm_start
        .import _kernel_warm_start

UART_DATA   = $D010
UART_STATUS = $D011

        .segment "ZEROPAGE"
ptr:    .res 2
savea:  .res 1

        .segment "CODE"

; void __fastcall__ os_putc(unsigned char value)
_os_putc:
        sta savea
putc_wait:
        lda UART_STATUS
        and #$01
        beq putc_wait
        lda savea
        sta UART_DATA
        rts

; unsigned char os_getc(void)
_os_getc:
getc_wait:
        lda UART_STATUS
        and #$02
        beq getc_wait
        lda UART_DATA
        rts

; void __fastcall__ os_puts(const char* text)
_os_puts:
        sta ptr
        stx ptr+1
        ldy #$00
puts_loop:
        lda (ptr),y
        beq puts_done
        jsr _os_putc
        iny
        bne puts_loop
puts_done:
        rts

_os_newline:
        lda #$0A
        jmp _os_putc

_os_warm_start:
        jmp _kernel_warm_start
