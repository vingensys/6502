#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cpu/cpu.h"
#include "mem/mem.h"

#ifndef TEST_BIN
#define TEST_BIN "build/tests/6502_cpu_batch5_test.bin"
#endif
#define STATUS_U 0x20

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

static void expect_common_flags(const char* test, uint8_t status) {
    expect_u8(test, "C", cpu_extract_sr(C), (status >> C) & 1);
    expect_u8(test, "Z", cpu_extract_sr(Z), (status >> Z) & 1);
    expect_u8(test, "I", cpu_extract_sr(I), (status >> I) & 1);
    expect_u8(test, "D", cpu_extract_sr(D), (status >> D) & 1);
    expect_u8(test, "V", cpu_extract_sr(V), (status >> V) & 1);
    expect_u8(test, "N", cpu_extract_sr(N), (status >> N) & 1);
    expect_u8(test, "unused bit 5", cpu_extract_sr(5), 1);
    expect_u8(test, "B normalized clear", cpu_extract_sr(B), 0);
}

static void expect_interrupt_stack(const char* test, uint16_t pc,
                                   uint8_t status, uint8_t want_b) {
    expect_u8(test, "SP after interrupt", cpu.sp, 0xFA);
    expect_u8(test, "pushed PC high", mem_read(0x01FD), pc >> 8);
    expect_u8(test, "pushed PC low", mem_read(0x01FC), pc & 0xFF);
    expect_u8(test, "pushed status bit 5", mem_read(0x01FB) & STATUS_U,
              STATUS_U);
    expect_u8(test, "pushed status B", (mem_read(0x01FB) >> B) & 1, want_b);
    expect_u8(test, "pushed status payload", mem_read(0x01FB) & status,
              status);
}

static void test_php_plp_status_normalization(void) {
    const char* name = "PHP/PLP status normalization";
    const uint8_t php[] = {0x08};
    const uint8_t plp[] = {0x28};
    uint8_t restored = (1 << C) | (1 << Z) | (1 << I) | (1 << D) |
                       (1 << V) | (1 << N);

    load_program(php, sizeof(php));
    cpu.sr = restored;
    step(1);
    expect_u8(name, "SP after PHP", cpu.sp, 0xFC);
    expect_u8(name, "PHP sets B", mem_read(0x01FD) & (1 << B), (1 << B));
    expect_u8(name, "PHP sets bit 5", mem_read(0x01FD) & STATUS_U, STATUS_U);
    expect_u8(name, "PHP payload", mem_read(0x01FD) & restored, restored);

    load_program(plp, sizeof(plp));
    cpu.sp = 0xFC;
    mem_write(0x01FD, restored | STATUS_U | (1 << B));
    step(1);
    expect_common_flags(name, restored);
}

static void test_brk_vector_and_stack(void) {
    const char* name = "BRK vector and stack";
    const uint8_t program[] = {0x00, 0xEA};
    uint8_t payload = (1 << C) | (1 << D) | (1 << V) | (1 << N);

    load_program(program, sizeof(program));
    mem_set_vector(MEM_VECTOR_IRQ, 0x3456);
    cpu.sr = payload;
    step(1);

    expect_u16(name, "PC vector", cpu.pc, 0x3456);
    expect_interrupt_stack(name, 0x8002, payload, 1);
    expect_u8(name, "I set after BRK", cpu_extract_sr(I), 1);
    expect_u8(name, "B clear internally", cpu_extract_sr(B), 0);
    expect_u16(name, "cycles", (uint16_t)cycles(), 7);
}

static void test_rti_synthetic_frame(void) {
    const char* name = "RTI synthetic frame";
    const uint8_t program[] = {0x40};
    uint8_t status = (1 << C) | (1 << Z) | (1 << D) | (1 << V) | (1 << N);

    load_program(program, sizeof(program));
    cpu.sp = 0xFA;
    mem_write(0x01FB, status | STATUS_U | (1 << B));
    mem_write(0x01FC, 0x34);
    mem_write(0x01FD, 0x12);
    step(1);

    expect_u16(name, "PC", cpu.pc, 0x1234);
    expect_u8(name, "SP restored", cpu.sp, 0xFD);
    expect_common_flags(name, status);
}

static void test_irq_ignored_when_disabled(void) {
    const char* name = "IRQ ignored when I set";
    const uint8_t program[] = {0xEA};

    load_program(program, sizeof(program));
    mem_set_vector(MEM_VECTOR_IRQ, 0x3456);
    cpu_mod_sr(I, 1);
    cpu_request_irq();
    step(1);

    expect_u16(name, "PC executed NOP", cpu.pc, 0x8001);
    expect_u8(name, "SP unchanged", cpu.sp, 0xFD);
}

static void test_irq_serviced_when_enabled(void) {
    const char* name = "IRQ serviced when I clear";
    const uint8_t program[] = {0xEA};
    uint8_t payload = (1 << C) | (1 << D) | (1 << V) | (1 << N);

    load_program(program, sizeof(program));
    mem_set_vector(MEM_VECTOR_IRQ, 0x3456);
    cpu.sr = payload;
    cpu_request_irq();
    step(1);

    expect_u16(name, "PC vector", cpu.pc, 0x3456);
    expect_interrupt_stack(name, 0x8000, payload, 0);
    expect_u8(name, "I set after IRQ", cpu_extract_sr(I), 1);
    expect_u16(name, "cycles", (uint16_t)cycles(), 7);
}

static void test_nmi_serviced_when_disabled(void) {
    const char* name = "NMI serviced when I set";
    const uint8_t program[] = {0xEA};
    uint8_t payload = (1 << C) | (1 << I) | (1 << D) | (1 << V) | (1 << N);

    load_program(program, sizeof(program));
    mem_set_vector(MEM_VECTOR_NMI, 0x4567);
    cpu.sr = payload;
    cpu_request_nmi();
    step(1);

    expect_u16(name, "PC vector", cpu.pc, 0x4567);
    expect_interrupt_stack(name, 0x8000, payload, 0);
    expect_u8(name, "I set after NMI", cpu_extract_sr(I), 1);
    expect_u16(name, "cycles", (uint16_t)cycles(), 7);
}

int main(void) {
    test_php_plp_status_normalization();
    test_brk_vector_and_stack();
    test_rti_synthetic_frame();
    test_irq_ignored_when_disabled();
    test_irq_serviced_when_enabled();
    test_nmi_serviced_when_disabled();

    if (failures != 0) {
        fprintf(stderr, "%u CPU Batch 5 test failure(s)\n", failures);
        return 1;
    }

    printf("CPU Batch 5 tests passed\n");
    return 0;
}
