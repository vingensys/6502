#!/usr/bin/env python3
"""Build the echo_upper UART cartridge without requiring an external assembler."""

from pathlib import Path

BASE = 0x8000
UART_DATA = 0xD010
UART_STATUS = 0xD011
ZP_OUT = 0x00
ZP_STR = 0x02
OUT = Path(__file__).with_name("echo_upper.bin")


class Asm:
    def __init__(self):
        self.code = bytearray()
        self.labels = {}
        self.fixups = []

    @property
    def pc(self):
        return BASE + len(self.code)

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

    def rel_fix(self, label):
        self.fixups.append(("rel", len(self.code), label))
        self.b(0)

    def lda_imm(self, value): self.b(0xA9, value)
    def lda_abs(self, addr): self.b(0xAD, addr, addr >> 8)
    def sta_abs(self, addr): self.b(0x8D, addr, addr >> 8)
    def sta_zp(self, addr): self.b(0x85, addr)
    def lda_izy(self, addr): self.b(0xB1, addr)
    def and_imm(self, value): self.b(0x29, value)
    def cmp_imm(self, value): self.b(0xC9, value)
    def sbc_imm(self, value): self.b(0xE9, value)
    def ldy_imm(self, value): self.b(0xA0, value)
    def iny(self): self.b(0xC8)
    def sec(self): self.b(0x38)
    def beq(self, label): self.b(0xF0); self.rel_fix(label)
    def bmi(self, label): self.b(0x30); self.rel_fix(label)
    def bne(self, label): self.b(0xD0); self.rel_fix(label)
    def jsr(self, label): self.b(0x20); self.abs_fix(label)
    def jmp(self, label): self.b(0x4C); self.abs_fix(label)
    def rts(self): self.b(0x60)

    def resolve(self):
        for kind, pos, label in self.fixups:
            target = self.labels[label]
            if kind == "abs":
                self.code[pos] = target & 0xFF
                self.code[pos + 1] = target >> 8
            else:
                offset = target - (BASE + pos + 1)
                if not -128 <= offset <= 127:
                    raise ValueError(f"branch to {label} out of range: {offset}")
                self.code[pos] = offset & 0xFF


a = Asm()
a.lda_imm(0)
greeting_lo_patch = len(a.code) - 1
a.sta_zp(ZP_STR)
a.lda_imm(0)
greeting_hi_patch = len(a.code) - 1
a.sta_zp(ZP_STR + 1)
a.jsr("print_greeting")

a.label("loop")
a.jsr("getc")
a.cmp_imm(0x0D)
a.beq("newline")
a.cmp_imm(0x0A)
a.beq("newline")
a.cmp_imm(ord("a"))
a.bmi("echo")
a.cmp_imm(ord("{"))
a.bmi("make_upper")

a.label("echo")
a.jsr("putc")
a.jmp("loop")

a.label("make_upper")
a.sec()
a.sbc_imm(0x20)
a.jsr("putc")
a.jmp("loop")

a.label("newline")
a.lda_imm(0x0A)
a.jsr("putc")
a.jmp("loop")

a.label("print_greeting")
a.ldy_imm(0)
a.label("greeting_loop")
a.lda_izy(ZP_STR)
a.beq("greeting_done")
a.jsr("putc")
a.iny()
a.bne("greeting_loop")
a.label("greeting_done")
a.rts()

a.label("putc")
a.sta_zp(ZP_OUT)
a.label("putc_wait")
a.lda_abs(UART_STATUS)
a.and_imm(0x01)
a.beq("putc_wait")
a.lda_abs(ZP_OUT)
a.sta_abs(UART_DATA)
a.rts()

a.label("getc")
a.label("getc_wait")
a.lda_abs(UART_STATUS)
a.and_imm(0x02)
a.beq("getc_wait")
a.lda_abs(UART_DATA)
a.rts()

a.label("greeting")
a.zstr("ECHO UPPER READY\nTYPE TEXT:\n")

a.resolve()
a.code[greeting_lo_patch] = a.labels["greeting"] & 0xFF
a.code[greeting_hi_patch] = a.labels["greeting"] >> 8

OUT.write_bytes(a.code)
print(f"wrote {OUT}")
