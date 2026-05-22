; Tiny monitor ROM for the emulator.
; Built by build_monitor.py into monitor.bin and loaded at $E000.
;
; UART:
;   $D010 = data
;   $D011 = status, bit 0 TX ready, bit 1 RX ready
;
; Commands:
;   ?       help
;   Mhhhh   dump 16 bytes
;   Whhhhbb write byte
;   Ghhhh   jump to address
;   R       re-enter monitor
;   Rhhhh   legacy jump alias
;   Bhhhh   set one breakpoint
;   C       clear breakpoint
;   L       list breakpoint

        .org $E000

reset:
        ldx #$ff
        txs
        jsr print_banner

main:
        jsr prompt
        jsr read_line
        jsr handle_command
        jmp main
