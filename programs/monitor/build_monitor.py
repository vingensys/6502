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
ZP_BP_ACTIVE = 0x09
ZP_BP_LO = 0x0A
ZP_BP_HI = 0x0B
ZP_BP_BYTE = 0x0C
ZP_RET_LO = 0x0D
ZP_RET_HI = 0x0E
ZP_LAST_DUMP_LO = 0x0F
ZP_LAST_DUMP_HI = 0x10
ZP_END_LO = 0x11
ZP_END_HI = 0x12
CMD = 0x0200
UART_DATA = 0xD010
UART_STATUS = 0xD011


class Asm:
    def __init__(self, base):
        self.base = base
        self.code = bytearray()
        self.labels = {}
        self.fixups = []
        self._branch_id = 0

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
    def cmp_zp(self, addr): self.b(0xC5, addr)
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
    def tsx(self): self.b(0xBA)
    def txs(self): self.b(0x9A)
    def sec(self): self.b(0x38)
    def clc(self): self.b(0x18)
    def cld(self): self.b(0xD8)
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
    def bcc(self, label):
        self.b(0x90)
        self.rel_fix(label)
    def bmi(self, label):
        self.b(0x30)
        self.rel_fix(label)
    def bne_abs(self, label):
        skip = f"bne_abs_skip_{self._branch_id}"
        self._branch_id += 1
        self.beq(skip)
        self.jmp_abs(label)
        self.label(skip)
    def beq_abs(self, label):
        skip = f"beq_abs_skip_{self._branch_id}"
        self._branch_id += 1
        self.bne(skip)
        self.jmp_abs(label)
        self.label(skip)
    def jump_if_a_eq(self, value, label):
        skip = f"cmp_abs_skip_{self._branch_id}"
        self._branch_id += 1
        self.cmp_imm(value)
        self.bne(skip)
        self.jmp_abs(label)
        self.label(skip)

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
a.cld()
a.ldx_imm(0xFF)
a.txs()
a.lda_imm(0)
a.sta_zp(ZP_BP_ACTIVE)
a.sta_zp(ZP_LAST_DUMP_LO)
a.sta_zp(ZP_LAST_DUMP_HI)
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
a.jump_if_a_eq(ord("?"), "cmd_help")
a.jump_if_a_eq(ord("G"), "cmd_run")
a.jump_if_a_eq(ord("g"), "cmd_run")
a.jump_if_a_eq(ord("R"), "cmd_restart_or_legacy_run")
a.jump_if_a_eq(ord("r"), "cmd_restart_or_legacy_run")
a.jump_if_a_eq(ord("M"), "cmd_dump")
a.jump_if_a_eq(ord("m"), "cmd_dump")
a.jump_if_a_eq(ord("N"), "cmd_next_dump")
a.jump_if_a_eq(ord("n"), "cmd_next_dump")
a.jump_if_a_eq(ord("W"), "cmd_write")
a.jump_if_a_eq(ord("w"), "cmd_write")
a.jump_if_a_eq(ord("B"), "cmd_break_set")
a.jump_if_a_eq(ord("b"), "cmd_break_set")
a.jump_if_a_eq(ord("C"), "cmd_break_clear")
a.jump_if_a_eq(ord("c"), "cmd_break_clear")
a.jump_if_a_eq(ord("L"), "cmd_break_list")
a.jump_if_a_eq(ord("l"), "cmd_break_list")
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
a.bne_abs("cmd_err")
a.jmp_ind(ZP_PTR)

a.label("cmd_dump")
a.jsr("parse_addr")
a.cmp_imm(0)
a.bne_abs("cmd_err")
a.lda_abs(CMD + 5)
a.cmp_imm(ord("."))
a.beq("cmd_dump_range")
a.jsr("dump_line")
a.jsr("remember_next_dump")
a.rts()

a.label("cmd_dump_range")
a.jsr("parse_range_end")
a.cmp_imm(0)
a.bne_abs("cmd_err")
a.label("dump_range_loop")
a.jsr("dump_line")
a.jsr("dump_line_reaches_end")
a.cmp_imm(0)
a.bne("dump_range_done")
a.jsr("advance_dump_ptr")
a.jmp_abs("dump_range_loop")
a.label("dump_range_done")
a.jsr("remember_next_dump")
a.rts()

a.label("cmd_next_dump")
a.lda_zp(ZP_LAST_DUMP_LO)
a.sta_zp(ZP_PTR)
a.lda_zp(ZP_LAST_DUMP_HI)
a.sta_zp(ZP_PTR_HI)
a.jsr("dump_line")
a.jsr("remember_next_dump")
a.rts()

a.label("dump_line")
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

a.label("advance_dump_ptr")
a.clc()
a.lda_zp(ZP_PTR)
a.adc_imm(16)
a.sta_zp(ZP_PTR)
a.lda_zp(ZP_PTR_HI)
a.adc_imm(0)
a.sta_zp(ZP_PTR_HI)
a.rts()

a.label("remember_next_dump")
a.clc()
a.lda_zp(ZP_PTR)
a.adc_imm(16)
a.sta_zp(ZP_LAST_DUMP_LO)
a.lda_zp(ZP_PTR_HI)
a.adc_imm(0)
a.sta_zp(ZP_LAST_DUMP_HI)
a.rts()

a.label("dump_line_reaches_end")
a.clc()
a.lda_zp(ZP_PTR)
a.adc_imm(15)
a.sta_zp(ZP_TMP)
a.lda_zp(ZP_PTR_HI)
a.adc_imm(0)
a.sta_zp(ZP_VALUE)
a.lda_zp(ZP_VALUE)
a.cmp_zp(ZP_END_HI)
a.bcc("dump_line_not_done")
a.bne("dump_line_done")
a.lda_zp(ZP_TMP)
a.cmp_zp(ZP_END_LO)
a.bcc("dump_line_not_done")
a.label("dump_line_done")
a.lda_imm(1)
a.rts()
a.label("dump_line_not_done")
a.lda_imm(0)
a.rts()

a.label("cmd_write")
a.jsr("parse_addr")
a.cmp_imm(0)
a.bne_abs("cmd_err")
a.jsr("parse_byte")
a.cmp_imm(0)
a.bne_abs("cmd_err")
a.ldy_imm(0)
a.lda_zp(ZP_VALUE)
a.sta_izy(ZP_PTR)
a.jsr("print_addr")
a.lda_imm(ord("="))
a.jsr("putc")
a.lda_zp(ZP_VALUE)
a.jsr("print_hex_byte")
a.lda_imm(ord(" "))
a.jsr("putc")
a.jmp_abs("cmd_ok")

a.label("cmd_break_set")
a.jsr("parse_addr")
a.cmp_imm(0)
a.bne_abs("cmd_err")
a.jsr("restore_breakpoint_if_active")
a.ldy_imm(0)
a.lda_izy(ZP_PTR)
a.sta_zp(ZP_BP_BYTE)
a.lda_zp(ZP_PTR)
a.sta_zp(ZP_BP_LO)
a.lda_zp(ZP_PTR_HI)
a.sta_zp(ZP_BP_HI)
a.lda_imm(1)
a.sta_zp(ZP_BP_ACTIVE)
a.lda_imm(0)
a.sta_izy(ZP_PTR)
a.jmp_abs("cmd_ok")

a.label("cmd_break_clear")
a.lda_zp(ZP_BP_ACTIVE)
a.beq("cmd_none")
a.jsr("restore_breakpoint_if_active")
a.jmp_abs("cmd_ok")

a.label("cmd_break_list")
a.lda_zp(ZP_BP_ACTIVE)
a.beq("cmd_none")
a.lda_imm(ord("B"))
a.jsr("putc")
a.lda_zp(ZP_BP_LO)
a.sta_zp(ZP_PTR)
a.lda_zp(ZP_BP_HI)
a.sta_zp(ZP_PTR_HI)
a.jsr("print_addr")
a.jsr("newline")
a.rts()

a.label("cmd_err")
a.lda_imm_lo("err")
a.sta_zp(ZP_STR)
a.lda_imm_hi("err")
a.sta_zp(ZP_STR + 1)
a.jsr("puts")
a.rts()

a.label("cmd_ok")
a.lda_imm_lo("ok")
a.sta_zp(ZP_STR)
a.lda_imm_hi("ok")
a.sta_zp(ZP_STR + 1)
a.jsr("puts")
a.rts()

a.label("cmd_none")
a.lda_imm_lo("none")
a.sta_zp(ZP_STR)
a.lda_imm_hi("none")
a.sta_zp(ZP_STR + 1)
a.jsr("puts")
a.rts()

a.label("restore_breakpoint_if_active")
a.lda_zp(ZP_BP_ACTIVE)
a.beq("restore_breakpoint_done")
a.lda_zp(ZP_BP_LO)
a.sta_zp(ZP_PTR)
a.lda_zp(ZP_BP_HI)
a.sta_zp(ZP_PTR_HI)
a.ldy_imm(0)
a.lda_zp(ZP_BP_BYTE)
a.sta_izy(ZP_PTR)
a.lda_imm(0)
a.sta_zp(ZP_BP_ACTIVE)
a.label("restore_breakpoint_done")
a.rts()

a.label("brk_handler")
a.cld()
a.tsx()
a.inx()
a.inx()
a.lda_abx(0x0100)
a.sta_zp(ZP_RET_LO)
a.inx()
a.lda_abx(0x0100)
a.sta_zp(ZP_RET_HI)
a.sec()
a.lda_zp(ZP_RET_LO)
a.sbc_imm(2)
a.sta_zp(ZP_PTR)
a.lda_zp(ZP_RET_HI)
a.sbc_imm(0)
a.sta_zp(ZP_PTR_HI)
a.lda_zp(ZP_BP_ACTIVE)
a.beq("brk_plain")
a.lda_zp(ZP_PTR)
a.cmp_zp(ZP_BP_LO)
a.bne("brk_plain")
a.lda_zp(ZP_PTR_HI)
a.cmp_zp(ZP_BP_HI)
a.bne("brk_plain")
a.jsr("restore_breakpoint_if_active")
a.lda_imm_lo("brk_hit")
a.sta_zp(ZP_STR)
a.lda_imm_hi("brk_hit")
a.sta_zp(ZP_STR + 1)
a.jsr("puts")
a.jsr("print_addr")
a.jsr("newline")
a.jmp_abs("main")

a.label("brk_plain")
a.lda_imm_lo("brk_plain_msg")
a.sta_zp(ZP_STR)
a.lda_imm_hi("brk_plain_msg")
a.sta_zp(ZP_STR + 1)
a.jsr("puts")
a.jmp_abs("main")

a.label("parse_addr")
for offset, dest in ((1, ZP_PTR_HI), (3, ZP_PTR)):
    a.lda_abs(CMD + offset)
    a.jsr("hex_nibble")
    a.cmp_imm(0xFF)
    a.beq_abs("parse_bad")
    a.asl_a(); a.asl_a(); a.asl_a(); a.asl_a()
    a.sta_zp(dest)
    a.lda_abs(CMD + offset + 1)
    a.jsr("hex_nibble")
    a.cmp_imm(0xFF)
    a.beq_abs("parse_bad")
    a.ora_zp(dest)
    a.sta_zp(dest)
a.lda_imm(0)
a.rts()

a.label("parse_range_end")
for offset, dest in ((6, ZP_END_HI), (8, ZP_END_LO)):
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
a.zstr("TMS6502 MONITOR\nCPU: NMOS6502\nROM: E000-FFFF\nCART: 8000-BFFF\n")
a.label("help")
a.zstr(
    "?        help\n"
    "Mhhhh    dump 16 bytes from address\n"
    "Mhhhh.kkkk dump address range\n"
    "N        dump next 16 bytes\n"
    "Whhhhbb  write byte\n"
    "Ghhhh    go/run address\n"
    "Bhhhh    set breakpoint\n"
    "C        clear breakpoint\n"
    "L        list breakpoint\n"
    "R        restart monitor\n"
)
a.label("ok")
a.zstr("OK\n")
a.label("none")
a.zstr("NONE\n")
a.label("err")
a.zstr("ERR\n")
a.label("brk_hit")
a.zstr("BRK ")
a.label("brk_plain_msg")
a.zstr("BRK\n")
a.label("hex_chars")
a.text("0123456789ABCDEF")

a.resolve()
rom = bytearray([0x00] * 0x2000)
rom[:len(a.code)] = a.code
vectors = {
    0xFFFA: a.labels["reset"],
    0xFFFC: a.labels["reset"],
    0xFFFE: a.labels["brk_handler"],
}
for addr, target in vectors.items():
    offset = addr - BASE
    rom[offset] = target & 0xFF
    rom[offset + 1] = target >> 8

OUT.write_bytes(rom)
print(f"wrote {OUT} ({len(rom)} bytes, ${BASE:04X}-$FFFF)")
