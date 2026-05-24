        .import _main
        .importzp sp
        .export _start

        .segment "STARTUP"

_start:
        cld
        ldx #$ff
        txs
        lda #<$7fff
        sta sp
        lda #>$7fff
        sta sp+1
        jsr _main
        jmp $E100
