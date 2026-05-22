#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cpu/cpu.h"
#include "mem/mem.h"
#include "peripherals/uart.h"

#define TEST_BIN "/tmp/6502_cpu_batch3_test.bin"

uint8_t DEBUG = 0;

static unsigned failures = 0;

static void fail_u16(const char* test, const char* what, uint16_t got,
                     uint16_t want) {
    fprintf(stderr, "[FAIL] %s: %s got $%04X want $%04X\n", test, what, got,
            want);
    failures++;
}

static void fail_u8(const char* test, const char* what, uint8_t got,
                    uint8_t want) {
    fprintf(stderr, "[FAIL] %s: %s got $%02X want $%02X\n", test, what, got,
            want);
    failures++;
}

static void expect_u16(const char* test, const char* what, uint16_t got,
                       uint16_t want) {
    if (got != want) fail_u16(test, what, got, want);
}

static void expect_u8(const char* test, const char* what, uint8_t got,
                      uint8_t want) {
    if (got != want) fail_u8(test, what, got, want);
}

static void write_program(const uint8_t* program, size_t size) {
    FILE* fp = fopen(TEST_BIN, "wb");
    if (fp == NULL) {
        perror(TEST_BIN);
        exit(1);
    }
    if (fwrite(program, 1, size, fp) != size) {
        perror("fwrite");
        fclose(fp);
        exit(1);
    }
    fclose(fp);
}

static void load_program(const uint8_t* program, size_t size) {
    write_program(program, size);
    mem_init(TEST_BIN);
    cpu_init();
    cpu_reset();
}

static void step(unsigned count) {
    for (unsigned i = 0; i < count; i++) cpu_exec();
}

static uint64_t cycles(void) {
    return cpu_get_execution_stats().total_cycles_executed;
}

static void expect_zn(const char* test, uint8_t value) {
    expect_u8(test, "Z flag", cpu_extract_sr(Z), value == 0);
    expect_u8(test, "N flag", cpu_extract_sr(N), (value & 0x80) != 0);
}

static void test_lda_modes(void) {
    const uint8_t imm[] = {0xA9, 0x80};
    const uint8_t zp[] = {0xA5, 0x10};
    const uint8_t zpx[] = {0xA2, 0x20, 0xB5, 0xF0};
    const uint8_t abs[] = {0xAD, 0x00, 0x03};
    const uint8_t abx[] = {0xA2, 0x10, 0xBD, 0xF0, 0x02};
    const uint8_t aby[] = {0xA0, 0x10, 0xB9, 0xF0, 0x02};
    const uint8_t izx[] = {0xA2, 0x20, 0xA1, 0xE0};
    const uint8_t izy[] = {0xA0, 0x10, 0xB1, 0xFF};

    load_program(imm, sizeof(imm));
    step(1);
    expect_u8("LDA immediate", "A", cpu.ac, 0x80);
    expect_zn("LDA immediate", 0x80);

    load_program(zp, sizeof(zp));
    mem_write(0x0010, 0x00);
    step(1);
    expect_u8("LDA zero page", "A", cpu.ac, 0x00);
    expect_zn("LDA zero page", 0x00);

    load_program(zpx, sizeof(zpx));
    mem_write(0x0010, 0x80);
    step(2);
    expect_u8("LDA zero page,X wrap", "A", cpu.ac, 0x80);
    expect_zn("LDA zero page,X wrap", 0x80);

    load_program(abs, sizeof(abs));
    mem_write(0x0300, 0x42);
    step(1);
    expect_u8("LDA absolute", "A", cpu.ac, 0x42);
    expect_zn("LDA absolute", 0x42);

    load_program(abx, sizeof(abx));
    mem_write(0x0300, 0x80);
    step(2);
    expect_u8("LDA absolute,X page cross", "A", cpu.ac, 0x80);
    expect_u16("LDA absolute,X page cross", "load cycles",
               (uint16_t)(cycles() - 2), 5);
    expect_zn("LDA absolute,X page cross", 0x80);

    load_program(aby, sizeof(aby));
    mem_write(0x0300, 0x00);
    step(2);
    expect_u8("LDA absolute,Y page cross", "A", cpu.ac, 0x00);
    expect_u16("LDA absolute,Y page cross", "load cycles",
               (uint16_t)(cycles() - 2), 5);
    expect_zn("LDA absolute,Y page cross", 0x00);

    load_program(izx, sizeof(izx));
    mem_write(0x0000, 0x34);
    mem_write(0x0001, 0x03);
    mem_write(0x0334, 0x80);
    step(2);
    expect_u8("LDA (indirect,X) wrap", "A", cpu.ac, 0x80);
    expect_zn("LDA (indirect,X) wrap", 0x80);

    load_program(izy, sizeof(izy));
    mem_write(0x00FF, 0xF0);
    mem_write(0x0000, 0x02);
    mem_write(0x0300, 0x42);
    step(2);
    expect_u8("LDA (indirect),Y wrap/cross", "A", cpu.ac, 0x42);
    expect_u16("LDA (indirect),Y wrap/cross", "load cycles",
               (uint16_t)(cycles() - 2), 6);
    expect_zn("LDA (indirect),Y wrap/cross", 0x42);
}

static void test_ldx_ldy_modes(void) {
    const uint8_t ldx_imm[] = {0xA2, 0x80};
    const uint8_t ldx_zp[] = {0xA6, 0x10};
    const uint8_t ldx_zpy[] = {0xA0, 0x20, 0xB6, 0xF0};
    const uint8_t ldx_abs[] = {0xAE, 0x00, 0x03};
    const uint8_t ldx_aby[] = {0xA0, 0x10, 0xBE, 0xF0, 0x02};
    const uint8_t ldy_imm[] = {0xA0, 0x80};
    const uint8_t ldy_zp[] = {0xA4, 0x10};
    const uint8_t ldy_zpx[] = {0xA2, 0x20, 0xB4, 0xF0};
    const uint8_t ldy_abs[] = {0xAC, 0x00, 0x03};
    const uint8_t ldy_abx[] = {0xA2, 0x10, 0xBC, 0xF0, 0x02};

    load_program(ldx_imm, sizeof(ldx_imm));
    step(1);
    expect_u8("LDX immediate", "X", cpu.x, 0x80);
    expect_zn("LDX immediate", 0x80);

    load_program(ldx_zp, sizeof(ldx_zp));
    mem_write(0x0010, 0x00);
    step(1);
    expect_u8("LDX zero page", "X", cpu.x, 0x00);
    expect_zn("LDX zero page", 0x00);

    load_program(ldx_zpy, sizeof(ldx_zpy));
    mem_write(0x0010, 0x80);
    step(2);
    expect_u8("LDX zero page,Y wrap", "X", cpu.x, 0x80);
    expect_zn("LDX zero page,Y wrap", 0x80);

    load_program(ldx_abs, sizeof(ldx_abs));
    mem_write(0x0300, 0x42);
    step(1);
    expect_u8("LDX absolute", "X", cpu.x, 0x42);
    expect_zn("LDX absolute", 0x42);

    load_program(ldx_aby, sizeof(ldx_aby));
    mem_write(0x0300, 0x80);
    step(2);
    expect_u8("LDX absolute,Y page cross", "X", cpu.x, 0x80);
    expect_u16("LDX absolute,Y page cross", "load cycles",
               (uint16_t)(cycles() - 2), 5);
    expect_zn("LDX absolute,Y page cross", 0x80);

    load_program(ldy_imm, sizeof(ldy_imm));
    step(1);
    expect_u8("LDY immediate", "Y", cpu.y, 0x80);
    expect_zn("LDY immediate", 0x80);

    load_program(ldy_zp, sizeof(ldy_zp));
    mem_write(0x0010, 0x00);
    step(1);
    expect_u8("LDY zero page", "Y", cpu.y, 0x00);
    expect_zn("LDY zero page", 0x00);

    load_program(ldy_zpx, sizeof(ldy_zpx));
    mem_write(0x0010, 0x80);
    step(2);
    expect_u8("LDY zero page,X wrap", "Y", cpu.y, 0x80);
    expect_zn("LDY zero page,X wrap", 0x80);

    load_program(ldy_abs, sizeof(ldy_abs));
    mem_write(0x0300, 0x42);
    step(1);
    expect_u8("LDY absolute", "Y", cpu.y, 0x42);
    expect_zn("LDY absolute", 0x42);

    load_program(ldy_abx, sizeof(ldy_abx));
    mem_write(0x0300, 0x80);
    step(2);
    expect_u8("LDY absolute,X page cross", "Y", cpu.y, 0x80);
    expect_u16("LDY absolute,X page cross", "load cycles",
               (uint16_t)(cycles() - 2), 5);
    expect_zn("LDY absolute,X page cross", 0x80);
}

static void test_sta_modes(void) {
    const uint8_t zp[] = {0xA9, 0x55, 0x85, 0x20};
    const uint8_t zpx[] = {0xA9, 0x55, 0xA2, 0x20, 0x95, 0xF0};
    const uint8_t abs[] = {0xA9, 0x55, 0x8D, 0x00, 0x03};
    const uint8_t abx[] = {0xA9, 0x55, 0xA2, 0x10, 0x9D, 0xF0, 0x02};
    const uint8_t aby[] = {0xA9, 0x55, 0xA0, 0x10, 0x99, 0xF0, 0x02};
    const uint8_t izx[] = {0xA9, 0x55, 0xA2, 0x20, 0x81, 0xE0};
    const uint8_t izy[] = {0xA9, 0x55, 0xA0, 0x10, 0x91, 0xFF};
    uint8_t sr;

    load_program(zp, sizeof(zp));
    step(1);
    sr = cpu.sr;
    step(1);
    expect_u8("STA zero page", "stored", mem_read(0x0020), 0x55);
    expect_u8("STA zero page", "SR unchanged", cpu.sr, sr);

    load_program(zpx, sizeof(zpx));
    step(2);
    sr = cpu.sr;
    step(1);
    expect_u8("STA zero page,X wrap", "stored", mem_read(0x0010), 0x55);
    expect_u8("STA zero page,X wrap", "SR unchanged", cpu.sr, sr);

    load_program(abs, sizeof(abs));
    step(1);
    sr = cpu.sr;
    step(1);
    expect_u8("STA absolute", "stored", mem_read(0x0300), 0x55);
    expect_u8("STA absolute", "SR unchanged", cpu.sr, sr);

    load_program(abx, sizeof(abx));
    step(2);
    sr = cpu.sr;
    step(1);
    expect_u8("STA absolute,X page cross", "stored", mem_read(0x0300), 0x55);
    expect_u8("STA absolute,X page cross", "SR unchanged", cpu.sr, sr);
    expect_u16("STA absolute,X page cross", "store cycles",
               (uint16_t)(cycles() - 4), 5);

    load_program(aby, sizeof(aby));
    step(2);
    sr = cpu.sr;
    step(1);
    expect_u8("STA absolute,Y page cross", "stored", mem_read(0x0300), 0x55);
    expect_u8("STA absolute,Y page cross", "SR unchanged", cpu.sr, sr);
    expect_u16("STA absolute,Y page cross", "store cycles",
               (uint16_t)(cycles() - 4), 5);

    load_program(izx, sizeof(izx));
    mem_write(0x0000, 0x34);
    mem_write(0x0001, 0x03);
    step(2);
    sr = cpu.sr;
    step(1);
    expect_u8("STA (indirect,X) wrap", "stored", mem_read(0x0334), 0x55);
    expect_u8("STA (indirect,X) wrap", "SR unchanged", cpu.sr, sr);

    load_program(izy, sizeof(izy));
    mem_write(0x00FF, 0xF0);
    mem_write(0x0000, 0x02);
    step(2);
    sr = cpu.sr;
    step(1);
    expect_u8("STA (indirect),Y wrap", "stored", mem_read(0x0300), 0x55);
    expect_u8("STA (indirect),Y wrap", "SR unchanged", cpu.sr, sr);
}

static void test_stx_sty_modes(void) {
    const uint8_t stx_zp[] = {0xA2, 0x66, 0x86, 0x20};
    const uint8_t stx_zpy[] = {0xA2, 0x66, 0xA0, 0x20, 0x96, 0xF0};
    const uint8_t stx_abs[] = {0xA2, 0x66, 0x8E, 0x00, 0x03};
    const uint8_t sty_zp[] = {0xA0, 0x77, 0x84, 0x20};
    const uint8_t sty_zpx[] = {0xA0, 0x77, 0xA2, 0x20, 0x94, 0xF0};
    const uint8_t sty_abs[] = {0xA0, 0x77, 0x8C, 0x00, 0x03};
    uint8_t sr;

    load_program(stx_zp, sizeof(stx_zp));
    step(1);
    sr = cpu.sr;
    step(1);
    expect_u8("STX zero page", "stored", mem_read(0x0020), 0x66);
    expect_u8("STX zero page", "SR unchanged", cpu.sr, sr);

    load_program(stx_zpy, sizeof(stx_zpy));
    step(2);
    sr = cpu.sr;
    step(1);
    expect_u8("STX zero page,Y wrap", "stored", mem_read(0x0010), 0x66);
    expect_u8("STX zero page,Y wrap", "SR unchanged", cpu.sr, sr);

    load_program(stx_abs, sizeof(stx_abs));
    step(1);
    sr = cpu.sr;
    step(1);
    expect_u8("STX absolute", "stored", mem_read(0x0300), 0x66);
    expect_u8("STX absolute", "SR unchanged", cpu.sr, sr);

    load_program(sty_zp, sizeof(sty_zp));
    step(1);
    sr = cpu.sr;
    step(1);
    expect_u8("STY zero page", "stored", mem_read(0x0020), 0x77);
    expect_u8("STY zero page", "SR unchanged", cpu.sr, sr);

    load_program(sty_zpx, sizeof(sty_zpx));
    step(2);
    sr = cpu.sr;
    step(1);
    expect_u8("STY zero page,X wrap", "stored", mem_read(0x0010), 0x77);
    expect_u8("STY zero page,X wrap", "SR unchanged", cpu.sr, sr);

    load_program(sty_abs, sizeof(sty_abs));
    step(1);
    sr = cpu.sr;
    step(1);
    expect_u8("STY absolute", "stored", mem_read(0x0300), 0x77);
    expect_u8("STY absolute", "SR unchanged", cpu.sr, sr);
}

static void test_bus_behavior(void) {
    const uint8_t program[] = {0xEA};

    load_program(program, sizeof(program));

    mem_write(0x0200, 0x11);
    expect_u8("bus RAM", "write/read", mem_read(0x0200), 0x11);

    mem_write(0xC000, 0x22);
    expect_u8("bus expansion RAM", "write/read", mem_read(0xC000), 0x22);

    expect_u8("bus program ROM", "initial", mem_read(0x8000), 0xEA);
    mem_write(0x8000, 0x33);
    expect_u8("bus program ROM", "write ignored", mem_read(0x8000), 0xEA);

    expect_u8("bus kernel ROM", "initial", mem_read(0xE000), 0x00);
    mem_write(0xE000, 0x44);
    expect_u8("bus kernel ROM", "write ignored", mem_read(0xE000), 0x00);

    mem_write(MEM_UART_DATA, 'A');
    expect_u8("bus UART", "buffer length", (uint8_t)uart_get_buffer_len(), 1);
    expect_u8("bus UART", "buffer byte", (uint8_t)uart_get_buffer()[0], 'A');
    expect_u8("bus UART", "status TX ready", mem_read(MEM_UART_STATUS) & 0x01,
              0x01);
}

int main(void) {
    test_lda_modes();
    test_ldx_ldy_modes();
    test_sta_modes();
    test_stx_sty_modes();
    test_bus_behavior();

    if (failures != 0) {
        fprintf(stderr, "%u CPU Batch 3 test failure(s)\n", failures);
        return 1;
    }

    printf("CPU Batch 3 tests passed\n");
    return 0;
}
