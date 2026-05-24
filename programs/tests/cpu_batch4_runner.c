#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cpu/cpu.h"
#include "mem/mem.h"

#ifndef TEST_BIN
#define TEST_BIN "build/tests/6502_cpu_batch4_test.bin"
#endif
#define FLAG_MASK ((uint8_t)((1 << C) | (1 << Z) | (1 << I) | (1 << D) | \
                             (1 << B) | (1 << V) | (1 << N)))

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
}

static void test_logical_ops(void) {
    const uint8_t and_imm[] = {0x29, 0x0F};
    const uint8_t and_zpx[] = {0x35, 0xF0};
    const uint8_t ora_zp[] = {0x05, 0x10};
    const uint8_t ora_izx[] = {0x01, 0xE0};
    const uint8_t eor_abs[] = {0x4D, 0x00, 0x03};
    const uint8_t eor_izy[] = {0x51, 0xFF};

    load_program(and_imm, sizeof(and_imm));
    cpu.ac = 0xF0;
    cpu.sr = (1 << C) | (1 << V);
    step(1);
    expect_u8("AND immediate", "A", cpu.ac, 0x00);
    expect_flags("AND immediate", 1, 1, 1, 0);

    load_program(and_zpx, sizeof(and_zpx));
    cpu.ac = 0xF0;
    cpu.x = 0x20;
    cpu.sr = (1 << C) | (1 << V);
    mem_write(0x0010, 0x80);
    step(1);
    expect_u8("AND zero page,X", "A", cpu.ac, 0x80);
    expect_flags("AND zero page,X", 1, 0, 1, 1);

    load_program(ora_zp, sizeof(ora_zp));
    cpu.ac = 0x00;
    cpu.sr = (1 << C) | (1 << V);
    mem_write(0x0010, 0x80);
    step(1);
    expect_u8("ORA zero page", "A", cpu.ac, 0x80);
    expect_flags("ORA zero page", 1, 0, 1, 1);

    load_program(ora_izx, sizeof(ora_izx));
    cpu.ac = 0x00;
    cpu.x = 0x20;
    cpu.sr = (1 << C) | (1 << V);
    mem_write(0x0000, 0x34);
    mem_write(0x0001, 0x03);
    mem_write(0x0334, 0x00);
    step(1);
    expect_u8("ORA (indirect,X)", "A", cpu.ac, 0x00);
    expect_flags("ORA (indirect,X)", 1, 1, 1, 0);

    load_program(eor_abs, sizeof(eor_abs));
    cpu.ac = 0x55;
    cpu.sr = (1 << C) | (1 << V);
    mem_write(0x0300, 0x55);
    step(1);
    expect_u8("EOR absolute", "A", cpu.ac, 0x00);
    expect_flags("EOR absolute", 1, 1, 1, 0);

    load_program(eor_izy, sizeof(eor_izy));
    cpu.ac = 0x7F;
    cpu.y = 0x10;
    cpu.sr = (1 << C) | (1 << V);
    mem_write(0x00FF, 0xF0);
    mem_write(0x0000, 0x02);
    mem_write(0x0300, 0xFF);
    step(1);
    expect_u8("EOR (indirect),Y", "A", cpu.ac, 0x80);
    expect_flags("EOR (indirect),Y", 1, 0, 1, 1);
}

static void run_compare_case(const char* name, uint8_t opcode, uint8_t reg,
                             uint8_t operand, uint8_t want_c, uint8_t want_z,
                             uint8_t want_n) {
    uint8_t program[] = {opcode, operand};
    load_program(program, sizeof(program));
    if (opcode == 0xC9) cpu.ac = reg;
    if (opcode == 0xE0) cpu.x = reg;
    if (opcode == 0xC0) cpu.y = reg;
    step(1);
    expect_u8(name, "C flag", cpu_extract_sr(C), want_c);
    expect_u8(name, "Z flag", cpu_extract_sr(Z), want_z);
    expect_u8(name, "N flag", cpu_extract_sr(N), want_n);
}

static void test_comparisons(void) {
    run_compare_case("CMP equal", 0xC9, 0x20, 0x20, 1, 1, 0);
    run_compare_case("CMP greater", 0xC9, 0x30, 0x20, 1, 0, 0);
    run_compare_case("CMP less", 0xC9, 0x10, 0x20, 0, 0, 1);
    run_compare_case("CPX equal", 0xE0, 0x20, 0x20, 1, 1, 0);
    run_compare_case("CPX greater", 0xE0, 0x30, 0x20, 1, 0, 0);
    run_compare_case("CPX less", 0xE0, 0x10, 0x20, 0, 0, 1);
    run_compare_case("CPY equal", 0xC0, 0x20, 0x20, 1, 1, 0);
    run_compare_case("CPY greater", 0xC0, 0x30, 0x20, 1, 0, 0);
    run_compare_case("CPY less", 0xC0, 0x10, 0x20, 0, 0, 1);
}

static void test_adc_binary(void) {
    const uint8_t add_no_carry[] = {0x18, 0xA9, 0x10, 0x69, 0x20};
    const uint8_t add_incoming_carry[] = {0x38, 0xA9, 0x01, 0x69, 0x01};
    const uint8_t carry_zero[] = {0x18, 0xA9, 0xFF, 0x69, 0x01};
    const uint8_t negative_overflow[] = {0xD8, 0x18, 0xA9, 0x40, 0x69, 0x40};
    const uint8_t carry_overflow_zero[] = {0x18, 0xA9, 0x80, 0x69, 0x80};

    load_program(add_no_carry, sizeof(add_no_carry));
    step(3);
    expect_u8("ADC no carry", "A", cpu.ac, 0x30);
    expect_flags("ADC no carry", 0, 0, 0, 0);

    load_program(add_incoming_carry, sizeof(add_incoming_carry));
    step(3);
    expect_u8("ADC incoming carry", "A", cpu.ac, 0x03);
    expect_flags("ADC incoming carry", 0, 0, 0, 0);

    load_program(carry_zero, sizeof(carry_zero));
    step(3);
    expect_u8("ADC carry zero", "A", cpu.ac, 0x00);
    expect_flags("ADC carry zero", 1, 1, 0, 0);

    load_program(negative_overflow, sizeof(negative_overflow));
    step(4);
    expect_u8("ADC negative overflow", "A", cpu.ac, 0x80);
    expect_flags("ADC negative overflow", 0, 0, 1, 1);
    expect_u8("ADC negative overflow", "D flag", cpu_extract_sr(D), 0);

    load_program(carry_overflow_zero, sizeof(carry_overflow_zero));
    step(3);
    expect_u8("ADC carry overflow zero", "A", cpu.ac, 0x00);
    expect_flags("ADC carry overflow zero", 1, 1, 1, 0);
}

static void test_sbc_binary(void) {
    const uint8_t no_borrow[] = {0x38, 0xA9, 0x05, 0xE9, 0x03};
    const uint8_t incoming_borrow[] = {0x18, 0xA9, 0x05, 0xE9, 0x03};
    const uint8_t borrow_negative[] = {0x38, 0xA9, 0x03, 0xE9, 0x05};
    const uint8_t zero[] = {0x38, 0xA9, 0x05, 0xE9, 0x05};
    const uint8_t overflow[] = {0xD8, 0x38, 0xA9, 0x80, 0xE9, 0x01};

    load_program(no_borrow, sizeof(no_borrow));
    step(3);
    expect_u8("SBC no borrow", "A", cpu.ac, 0x02);
    expect_flags("SBC no borrow", 1, 0, 0, 0);

    load_program(incoming_borrow, sizeof(incoming_borrow));
    step(3);
    expect_u8("SBC incoming borrow", "A", cpu.ac, 0x01);
    expect_flags("SBC incoming borrow", 1, 0, 0, 0);

    load_program(borrow_negative, sizeof(borrow_negative));
    step(3);
    expect_u8("SBC borrow negative", "A", cpu.ac, 0xFE);
    expect_flags("SBC borrow negative", 0, 0, 0, 1);

    load_program(zero, sizeof(zero));
    step(3);
    expect_u8("SBC zero", "A", cpu.ac, 0x00);
    expect_flags("SBC zero", 1, 1, 0, 0);

    load_program(overflow, sizeof(overflow));
    step(4);
    expect_u8("SBC overflow", "A", cpu.ac, 0x7F);
    expect_flags("SBC overflow", 1, 0, 1, 0);
    expect_u8("SBC overflow", "D flag", cpu_extract_sr(D), 0);
}

static void test_shifts(void) {
    const uint8_t asl_a[] = {0xA9, 0x81, 0x0A};
    const uint8_t asl_mem[] = {0x06, 0x10};
    const uint8_t lsr_a[] = {0xA9, 0x01, 0x4A};
    const uint8_t lsr_mem[] = {0x46, 0x10};

    load_program(asl_a, sizeof(asl_a));
    step(2);
    expect_u8("ASL accumulator", "A", cpu.ac, 0x02);
    expect_flags("ASL accumulator", 1, 0, 0, 0);

    load_program(asl_mem, sizeof(asl_mem));
    mem_write(0x0010, 0x80);
    step(1);
    expect_u8("ASL memory", "memory", mem_read(0x0010), 0x00);
    expect_flags("ASL memory", 1, 1, 0, 0);

    load_program(lsr_a, sizeof(lsr_a));
    step(2);
    expect_u8("LSR accumulator", "A", cpu.ac, 0x00);
    expect_flags("LSR accumulator", 1, 1, 0, 0);

    load_program(lsr_mem, sizeof(lsr_mem));
    mem_write(0x0010, 0x80);
    step(1);
    expect_u8("LSR memory", "memory", mem_read(0x0010), 0x40);
    expect_flags("LSR memory", 0, 0, 0, 0);
}

static void test_rotates(void) {
    const uint8_t rol_a[] = {0x38, 0xA9, 0x80, 0x2A};
    const uint8_t rol_mem[] = {0x18, 0x26, 0x10};
    const uint8_t ror_a[] = {0x38, 0xA9, 0x01, 0x6A};
    const uint8_t ror_mem[] = {0x18, 0x66, 0x10};

    load_program(rol_a, sizeof(rol_a));
    step(3);
    expect_u8("ROL accumulator", "A", cpu.ac, 0x01);
    expect_flags("ROL accumulator", 1, 0, 0, 0);

    load_program(rol_mem, sizeof(rol_mem));
    mem_write(0x0010, 0x40);
    step(2);
    expect_u8("ROL memory", "memory", mem_read(0x0010), 0x80);
    expect_flags("ROL memory", 0, 0, 0, 1);

    load_program(ror_a, sizeof(ror_a));
    step(3);
    expect_u8("ROR accumulator", "A", cpu.ac, 0x80);
    expect_flags("ROR accumulator", 1, 0, 0, 1);

    load_program(ror_mem, sizeof(ror_mem));
    mem_write(0x0010, 0x01);
    step(2);
    expect_u8("ROR memory", "memory", mem_read(0x0010), 0x00);
    expect_flags("ROR memory", 1, 1, 0, 0);
}

static void test_inc_dec(void) {
    const uint8_t dec_zero[] = {0xC6, 0x10};
    const uint8_t dec_negative[] = {0xC6, 0x10};
    const uint8_t inc_zero[] = {0xE6, 0x10};
    const uint8_t inc_negative[] = {0xE6, 0x10};

    load_program(dec_zero, sizeof(dec_zero));
    mem_write(0x0010, 0x01);
    step(1);
    expect_u8("DEC zero", "memory", mem_read(0x0010), 0x00);
    expect_flags("DEC zero", 0, 1, 0, 0);

    load_program(dec_negative, sizeof(dec_negative));
    mem_write(0x0010, 0x00);
    step(1);
    expect_u8("DEC negative", "memory", mem_read(0x0010), 0xFF);
    expect_flags("DEC negative", 0, 0, 0, 1);

    load_program(inc_zero, sizeof(inc_zero));
    mem_write(0x0010, 0xFF);
    step(1);
    expect_u8("INC zero", "memory", mem_read(0x0010), 0x00);
    expect_flags("INC zero", 0, 1, 0, 0);

    load_program(inc_negative, sizeof(inc_negative));
    mem_write(0x0010, 0x7F);
    step(1);
    expect_u8("INC negative", "memory", mem_read(0x0010), 0x80);
    expect_flags("INC negative", 0, 0, 0, 1);
}

int main(void) {
    test_logical_ops();
    test_comparisons();
    test_adc_binary();
    test_sbc_binary();
    test_shifts();
    test_rotates();
    test_inc_dec();

    if (failures != 0) {
        fprintf(stderr, "%u CPU Batch 4 test failure(s)\n", failures);
        return 1;
    }

    printf("CPU Batch 4 tests passed\n");
    return 0;
}
