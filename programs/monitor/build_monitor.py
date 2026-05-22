#!/usr/bin/env python3
"""Build the tiny UART monitor ROM without requiring an external assembler."""

from pathlib import Path

BASE = 0xE000
OUT = Path(__file__).with_name("monitor.bin")

ZP_PTR = 0x02
ZP_PTR_HI = 0x03
ZP_TMP = 0x04
ZP_VALUE = 0x05
ZP_STR = 0x06
ZP_OUT = 0x08
CMD = 0x0200
UART_DATA = 0xD010
UART_STATUS = 0xD011


class Asm:
    def __init__(self, base):
        self.base = base
        self.code = bytearray()
        self.labels = {}
        self.fixups = []

    @property
    def pc(self):
        return self.base + len(self.code)

    def label(self, name):
        self.labels[name] = self.pc

    def b(self, *values):
        self.code.extend(v & 0xFF for v in values)

    def text(self, value):
        self.code.extend(value.encode("ascii"))

    def zstr(self, value):
        self.text(value)
        self.b(0)

    def abs_fix(self, label):
        self.fixups.append(("abs", len(self.code), label))
        self.b(0, 0)

    def imm_lo_fix(self, label):
        self.fixups.append(("lo", len(self.code), label))
        self.b(0)

    def imm_hi_fix(self, label):
        self.fixups.append(("hi", len(self.code), label))
        self.b(0)

    def rel_fix(self, label):
        self.fixups.append(("rel", len(self.code), label))
        self.b(0)

    def lda_imm(self, value): self.b(0xA9, value)
    def lda_imm_lo(self, label):
        self.b(0xA9)
        self.imm_lo_fix(label)
    def lda_imm_hi(self, label):
        self.b(0xA9)
        self.imm_hi_fix(label)
    def ldx_imm(self, value): self.b(0xA2, value)
    def ldy_imm(self, value): self.b(0xA0, value)
    def lda_zp(self, addr): self.b(0xA5, addr)
    def sta_zp(self, addr): self.b(0x85, addr)
    def ora_zp(self, addr): self.b(0x05, addr)
    def lda_abs(self, addr): self.b(0xAD, addr, addr >> 8)
    def sta_abs(self, addr): self.b(0x8D, addr, addr >> 8)
    def lda_abx(self, addr): self.b(0xBD, addr, addr >> 8)
    def sta_abx(self, addr): self.b(0x9D, addr, addr >> 8)
    def lda_izy(self, zp): self.b(0xB1, zp)
    def sta_izy(self, zp): self.b(0x91, zp)
    def cmp_imm(self, value): self.b(0xC9, value)
    def cpx_imm(self, value): self.b(0xE0, value)
    def cpy_imm(self, value): self.b(0xC0, value)
    def and_imm(self, value): self.b(0x29, value)
    def adc_imm(self, value): self.b(0x69, value)
    def sbc_imm(self, value): self.b(0xE9, value)
    def asl_a(self): self.b(0x0A)
    def lsr_a(self): self.b(0x4A)
    def inx(self): self.b(0xE8)
    def iny(self): self.b(0xC8)
    def tax(self): self.b(0xAA)
    def txs(self): self.b(0x9A)
    def sec(self): self.b(0x38)
    def clc(self): self.b(0x18)
    def rts(self): self.b(0x60)
    def jmp_abs(self, label):
        self.b(0x4C)
        self.abs_fix(label)
    def jmp_ind(self, addr): self.b(0x6C, addr, addr >> 8)
    def jsr(self, label):
        self.b(0x20)
        self.abs_fix(label)
    def beq(self, label):
        self.b(0xF0)
        self.rel_fix(label)
    def bne(self, label):
        self.b(0xD0)
        self.rel_fix(label)
    def bmi(self, label):
        self.b(0x30)
        self.rel_fix(label)

    def resolve(self):
        for kind, pos, label in self.fixups:
            target = self.labels[label]
            if kind == "abs":
                self.code[pos] = target & 0xFF
                self.code[pos + 1] = target >> 8
            elif kind == "lo":
                self.code[pos] = target & 0xFF
            elif kind == "hi":
                self.code[pos] = target >> 8
            else:
                offset = target - (self.base + pos + 1)
                if not -128 <= offset <= 127:
                    raise ValueError(f"branch to {label} out of range: {offset}")
                self.code[pos] = offset & 0xFF


a = Asm(BASE)

a.label("reset")
a.ldx_imm(0xFF)
a.txs()
a.jsr("print_banner")

a.label("main")
a.jsr("prompt")
a.jsr("read_line")
a.jsr("handle_command")
a.jmp_abs("main")

a.label("print_banner")
a.lda_imm_lo("banner")
a.sta_zp(ZP_STR)
a.lda_imm_hi("banner")
a.sta_zp(ZP_STR + 1)
a.jsr("puts")
a.rts()

a.label("prompt")
a.lda_imm(ord(">"))
a.jsr("putc")
a.rts()

a.label("read_line")
a.ldx_imm(0)
a.label("read_line_loop")
a.jsr("getc")
a.cmp_imm(0x0D)
a.beq("read_line_done")
a.cmp_imm(0x0A)
a.beq("read_line_done")
a.sta_abx(CMD)
a.jsr("putc")
a.inx()
a.cpx_imm(0x20)
a.bne("read_line_loop")
a.label("read_line_done")
a.lda_imm(0)
a.sta_abx(CMD)
a.jsr("newline")
a.rts()

a.label("handle_command")
a.lda_abs(CMD)
a.cmp_imm(ord("?"))
a.beq("cmd_help")
a.cmp_imm(ord("G"))
a.beq("cmd_run")
a.cmp_imm(ord("g"))
a.beq("cmd_run")
a.cmp_imm(ord("R"))
a.beq("cmd_restart_or_legacy_run")
a.cmp_imm(ord("r"))
a.beq("cmd_restart_or_legacy_run")
a.cmp_imm(ord("M"))
a.beq("cmd_dump")
a.cmp_imm(ord("m"))
a.beq("cmd_dump")
a.cmp_imm(ord("W"))
a.beq("cmd_write")
a.cmp_imm(ord("w"))
a.beq("cmd_write")
a.jmp_abs("cmd_err")

a.label("cmd_help")
a.lda_imm_lo("help")
a.sta_zp(ZP_STR)
a.lda_imm_hi("help")
a.sta_zp(ZP_STR + 1)
a.jsr("puts")
a.rts()

a.label("cmd_restart_or_legacy_run")
a.lda_abs(CMD + 1)
a.cmp_imm(0)
a.beq("cmd_restart")
a.jmp_abs("cmd_run")

a.label("cmd_restart")
a.jsr("print_banner")
a.rts()

a.label("cmd_run")
a.jsr("parse_addr")
a.cmp_imm(0)
a.bne("cmd_err")
a.jmp_ind(ZP_PTR)

a.label("cmd_dump")
a.jsr("parse_addr")
a.cmp_imm(0)
a.bne("cmd_err")
a.jsr("print_addr")
a.lda_imm(ord(":"))
a.jsr("putc")
a.lda_imm(ord(" "))
a.jsr("putc")
a.ldy_imm(0)
a.label("dump_loop")
a.lda_izy(ZP_PTR)
a.jsr("print_hex_byte")
a.lda_imm(ord(" "))
a.jsr("putc")
a.iny()
a.cpy_imm(16)
a.bne("dump_loop")
a.jsr("newline")
a.rts()

a.label("cmd_write")
a.jsr("parse_addr")
a.cmp_imm(0)
a.bne("cmd_err")
a.jsr("parse_byte")
a.cmp_imm(0)
a.bne("cmd_err")
a.ldy_imm(0)
a.lda_zp(ZP_VALUE)
a.sta_izy(ZP_PTR)
a.lda_imm_lo("ok")
a.sta_zp(ZP_STR)
a.lda_imm_hi("ok")
a.sta_zp(ZP_STR + 1)
a.jsr("puts")
a.rts()

a.label("cmd_err")
a.lda_imm_lo("err")
a.sta_zp(ZP_STR)
a.lda_imm_hi("err")
a.sta_zp(ZP_STR + 1)
a.jsr("puts")
a.rts()

a.label("parse_addr")
for offset, dest in ((1, ZP_PTR_HI), (3, ZP_PTR)):
    a.lda_abs(CMD + offset)
    a.jsr("hex_nibble")
    a.cmp_imm(0xFF)
    a.beq("parse_bad")
    a.asl_a(); a.asl_a(); a.asl_a(); a.asl_a()
    a.sta_zp(dest)
    a.lda_abs(CMD + offset + 1)
    a.jsr("hex_nibble")
    a.cmp_imm(0xFF)
    a.beq("parse_bad")
    a.ora_zp(dest)
    a.sta_zp(dest)
a.lda_imm(0)
a.rts()

a.label("parse_byte")
a.lda_abs(CMD + 5)
a.jsr("hex_nibble")
a.cmp_imm(0xFF)
a.beq("parse_bad")
a.asl_a(); a.asl_a(); a.asl_a(); a.asl_a()
a.sta_zp(ZP_VALUE)
a.lda_abs(CMD + 6)
a.jsr("hex_nibble")
a.cmp_imm(0xFF)
a.beq("parse_bad")
a.ora_zp(ZP_VALUE)
a.sta_zp(ZP_VALUE)
a.lda_imm(0)
a.rts()

a.label("parse_bad")
a.lda_imm(1)
a.rts()

a.label("hex_nibble")
a.cmp_imm(ord("0"))
a.bmi("hex_bad")
a.cmp_imm(ord(":"))
a.bmi("hex_digit")
a.cmp_imm(ord("A"))
a.bmi("hex_lower_check")
a.cmp_imm(ord("G"))
a.bmi("hex_upper")
a.label("hex_lower_check")
a.cmp_imm(ord("a"))
a.bmi("hex_bad")
a.cmp_imm(ord("g"))
a.bmi("hex_lower")
a.label("hex_bad")
a.lda_imm(0xFF)
a.rts()
a.label("hex_digit")
a.and_imm(0x0F)
a.rts()
a.label("hex_upper")
a.and_imm(0x0F)
a.adc_imm(9)
a.rts()
a.label("hex_lower")
a.and_imm(0x0F)
a.adc_imm(9)
a.rts()

a.label("print_addr")
a.lda_zp(ZP_PTR_HI)
a.jsr("print_hex_byte")
a.lda_zp(ZP_PTR)
a.jsr("print_hex_byte")
a.rts()

a.label("print_hex_byte")
a.sta_zp(ZP_TMP)
a.lsr_a(); a.lsr_a(); a.lsr_a(); a.lsr_a()
a.jsr("print_hex_nibble")
a.lda_zp(ZP_TMP)
a.and_imm(0x0F)
a.jsr("print_hex_nibble")
a.rts()

a.label("print_hex_nibble")
a.tax()
a.lda_abx(0)  # patched below after labels are known
a.fixups.append(("abs", len(a.code) - 2, "hex_chars"))
a.jmp_abs("putc")

a.label("puts")
a.ldy_imm(0)
a.label("puts_loop")
a.lda_izy(ZP_STR)
a.beq("puts_done")
a.jsr("putc")
a.iny()
a.bne("puts_loop")
a.label("puts_done")
a.rts()

a.label("newline")
a.lda_imm(0x0A)
a.jsr("putc")
a.rts()

a.label("putc")
a.sta_zp(ZP_OUT)
a.label("putc_wait")
a.lda_abs(UART_STATUS)
a.and_imm(0x01)
a.beq("putc_wait")
a.lda_zp(ZP_OUT)
a.sta_abs(UART_DATA)
a.rts()

a.label("getc")
a.label("getc_wait")
a.lda_abs(UART_STATUS)
a.and_imm(0x02)
a.beq("getc_wait")
a.lda_abs(UART_DATA)
a.rts()

a.label("banner")
a.zstr("TMS6502 MONITOR\n")
a.label("help")
a.zstr("?:HELP Mhhhh Whhhhbb Ghhhh R\n")
a.label("ok")
a.zstr("OK\n")
a.label("err")
a.zstr("ERR\n")
a.label("hex_chars")
a.text("0123456789ABCDEF")

a.resolve()
OUT.write_bytes(a.code)
print(f"wrote {OUT} ({len(a.code)} bytes, ${BASE:04X}-${BASE + len(a.code) - 1:04X})")
