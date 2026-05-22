#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cpu/cpu.h"
#include "mem/mem.h"

#define TEST_BIN "/tmp/6502_cpu_batch6_decimal_test.bin"

uint8_t DEBUG = 0;

static unsigned failures = 0;

static void fail_u8(const char* test, const char* what, uint8_t got,
                    uint8_t want) {
    fprintf(stderr, "[FAIL] %s: %s got $%02X want $%02X\n", test, what, got,
            want);
    failures++;
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

static void expect_flags(const char* test, uint8_t c, uint8_t z, uint8_t v,
                         uint8_t n) {
    expect_u8(test, "C flag", cpu_extract_sr(C), c);
    expect_u8(test, "Z flag", cpu_extract_sr(Z), z);
    expect_u8(test, "V flag", cpu_extract_sr(V), v);
    expect_u8(test, "N flag", cpu_extract_sr(N), n);
    expect_u8(test, "D flag", cpu_extract_sr(D), 1);
}

static void test_adc_decimal_immediate(void) {
    const uint8_t add_15_27[] = {0xF8, 0x18, 0xA9, 0x15, 0x69, 0x27};
    const uint8_t add_99_01[] = {0xF8, 0x18, 0xA9, 0x99, 0x69, 0x01};
    const uint8_t add_with_carry[] = {0xF8, 0x38, 0xA9, 0x15, 0x69, 0x27};
    const uint8_t add_negative_v[] = {0xF8, 0x18, 0xA9, 0x50, 0x69, 0x30};

    load_program(add_15_27, sizeof(add_15_27));
    step(4);
    expect_u8("ADC decimal 15+27", "A", cpu.ac, 0x42);
    expect_flags("ADC decimal 15+27", 0, 0, 0, 0);

    load_program(add_99_01, sizeof(add_99_01));
    step(4);
    expect_u8("ADC decimal 99+01", "A", cpu.ac, 0x00);
    expect_flags("ADC decimal 99+01", 1, 1, 0, 0);

    load_program(add_with_carry, sizeof(add_with_carry));
    step(4);
    expect_u8("ADC decimal incoming carry", "A", cpu.ac, 0x43);
    expect_flags("ADC decimal incoming carry", 0, 0, 0, 0);

    load_program(add_negative_v, sizeof(add_negative_v));
    step(4);
    expect_u8("ADC decimal negative-looking", "A", cpu.ac, 0x80);
    expect_flags("ADC decimal negative-looking", 0, 0, 1, 1);
}

static void test_adc_decimal_addressing(void) {
    const uint8_t adc_zp[] = {0xF8, 0x18, 0xA9, 0x15, 0x65, 0x10};
    const uint8_t adc_abs[] = {0xF8, 0x18, 0xA9, 0x15, 0x6D, 0x00, 0x03};
    const uint8_t adc_abx[] = {0xF8, 0x18, 0xA2, 0x10, 0xA9, 0x15,
                               0x7D, 0xF0, 0x02};
    const uint8_t adc_izy[] = {0xF8, 0x18, 0xA0, 0x10, 0xA9, 0x15, 0x71, 0x80};

    load_program(adc_zp, sizeof(adc_zp));
    mem_write(0x0010, 0x27);
    step(4);
    expect_u8("ADC decimal zero page", "A", cpu.ac, 0x42);

    load_program(adc_abs, sizeof(adc_abs));
    mem_write(0x0300, 0x27);
    step(4);
    expect_u8("ADC decimal absolute", "A", cpu.ac, 0x42);

    load_program(adc_abx, sizeof(adc_abx));
    mem_write(0x0300, 0x27);
    step(5);
    expect_u8("ADC decimal absolute,X", "A", cpu.ac, 0x42);

    load_program(adc_izy, sizeof(adc_izy));
    mem_write(0x0080, 0xF0);
    mem_write(0x0081, 0x02);
    mem_write(0x0300, 0x27);
    step(5);
    expect_u8("ADC decimal (indirect),Y", "A", cpu.ac, 0x42);
}

static void test_sbc_decimal_immediate(void) {
    const uint8_t sub_42_27[] = {0xF8, 0x38, 0xA9, 0x42, 0xE9, 0x27};
    const uint8_t sub_00_01[] = {0xF8, 0x38, 0xA9, 0x00, 0xE9, 0x01};
    const uint8_t sub_no_borrow[] = {0xF8, 0x38, 0xA9, 0x42, 0xE9, 0x01};
    const uint8_t sub_incoming_borrow[] = {0xF8, 0x18, 0xA9, 0x42, 0xE9, 0x27};

    load_program(sub_42_27, sizeof(sub_42_27));
    step(4);
    expect_u8("SBC decimal 42-27", "A", cpu.ac, 0x15);
    expect_flags("SBC decimal 42-27", 1, 0, 0, 0);

    load_program(sub_00_01, sizeof(sub_00_01));
    step(4);
    expect_u8("SBC decimal 00-01", "A", cpu.ac, 0x99);
    expect_flags("SBC decimal 00-01", 0, 0, 0, 1);

    load_program(sub_no_borrow, sizeof(sub_no_borrow));
    step(4);
    expect_u8("SBC decimal no borrow", "A", cpu.ac, 0x41);
    expect_flags("SBC decimal no borrow", 1, 0, 0, 0);

    load_program(sub_incoming_borrow, sizeof(sub_incoming_borrow));
    step(4);
    expect_u8("SBC decimal incoming borrow", "A", cpu.ac, 0x14);
    expect_flags("SBC decimal incoming borrow", 1, 0, 0, 0);
}

static void test_sbc_decimal_addressing(void) {
    const uint8_t sbc_zp[] = {0xF8, 0x38, 0xA9, 0x42, 0xE5, 0x10};
    const uint8_t sbc_abs[] = {0xF8, 0x38, 0xA9, 0x42, 0xED, 0x00, 0x03};
    const uint8_t sbc_abx[] = {0xF8, 0x38, 0xA2, 0x10, 0xA9, 0x42,
                               0xFD, 0xF0, 0x02};
    const uint8_t sbc_izx[] = {0xF8, 0x38, 0xA2, 0x20, 0xA9, 0x42, 0xE1, 0xE0};

    load_program(sbc_zp, sizeof(sbc_zp));
    mem_write(0x0010, 0x27);
    step(4);
    expect_u8("SBC decimal zero page", "A", cpu.ac, 0x15);

    load_program(sbc_abs, sizeof(sbc_abs));
    mem_write(0x0300, 0x27);
    step(4);
    expect_u8("SBC decimal absolute", "A", cpu.ac, 0x15);

    load_program(sbc_abx, sizeof(sbc_abx));
    mem_write(0x0300, 0x27);
    step(5);
    expect_u8("SBC decimal absolute,X", "A", cpu.ac, 0x15);

    load_program(sbc_izx, sizeof(sbc_izx));
    mem_write(0x0000, 0x00);
    mem_write(0x0001, 0x03);
    mem_write(0x0300, 0x27);
    step(5);
    expect_u8("SBC decimal (indirect,X)", "A", cpu.ac, 0x15);
}

int main(void) {
    test_adc_decimal_immediate();
    test_adc_decimal_addressing();
    test_sbc_decimal_immediate();
    test_sbc_decimal_addressing();

    if (failures != 0) {
        fprintf(stderr, "%u CPU Batch 6A decimal test failure(s)\n", failures);
        return 1;
    }

    printf("CPU Batch 6A decimal tests passed\n");
    return 0;
}
