        .import _start

        .segment "VECTORS"

        .word _start     ; NMI
        .word _start     ; RESET
        .word _start     ; IRQ/BRK
