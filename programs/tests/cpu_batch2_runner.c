#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cpu/cpu.h"
#include "mem/mem.h"

#define TEST_BIN "/tmp/6502_cpu_batch2_test.bin"

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

static void test_jsr_rts(void) {
    const char* name = "JSR/RTS";
    uint8_t program[0x20] = {
        0x20, 0x10, 0x80,       /* $8000: JSR $8010 */
        0x8D, 0x20, 0x00,       /* $8003: STA $0020 */
        0xEA,                   /* $8006: NOP */
        [0x10] = 0xA9, [0x11] = 0x42, /* $8010: LDA #$42 */
        [0x12] = 0x60                 /* $8012: RTS */
    };

    load_program(program, sizeof(program));
    step(1);
    expect_u16(name, "PC after JSR", cpu.pc, 0x8010);
    expect_u8(name, "SP after JSR", cpu.sp, 0xFB);
    expect_u8(name, "return high byte", mem_read(0x01FD), 0x80);
    expect_u8(name, "return low byte", mem_read(0x01FC), 0x02);

    step(3);
    expect_u16(name, "PC after RTS and STA", cpu.pc, 0x8006);
    expect_u8(name, "A after subroutine", cpu.ac, 0x42);
    expect_u8(name, "stored result", mem_read(0x0020), 0x42);
    expect_u8(name, "SP restored", cpu.sp, 0xFD);
}

static void test_nested_jsr(void) {
    const char* name = "nested JSR/RTS";
    uint8_t program[0x30] = {
        0x20, 0x10, 0x80,             /* $8000: JSR $8010 */
        0x85, 0x21,                   /* $8003: STA $21 */
        [0x10] = 0x20, [0x11] = 0x20, [0x12] = 0x80, /* JSR $8020 */
        [0x13] = 0x60,                              /* RTS */
        [0x20] = 0xA9, [0x21] = 0x77,               /* LDA #$77 */
        [0x22] = 0x60                               /* RTS */
    };

    load_program(program, sizeof(program));
    step(6);
    expect_u16(name, "PC after nested returns", cpu.pc, 0x8005);
    expect_u8(name, "A from nested subroutine", cpu.ac, 0x77);
    expect_u8(name, "stored result", mem_read(0x0021), 0x77);
    expect_u8(name, "SP restored", cpu.sp, 0xFD);
}

static void test_pha_pla(void) {
    const char* name = "PHA/PLA";
    const uint8_t program[] = {
        0xA9, 0x5A, /* LDA #$5A */
        0x48,       /* PHA */
        0xA9, 0x00, /* LDA #$00 */
        0x68,       /* PLA */
        0x85, 0x22  /* STA $22 */
    };

    load_program(program, sizeof(program));
    step(2);
    expect_u8(name, "SP after PHA", cpu.sp, 0xFC);
    expect_u8(name, "stack page write", mem_read(0x01FD), 0x5A);
    step(3);
    expect_u8(name, "A after PLA", cpu.ac, 0x5A);
    expect_u8(name, "stored result", mem_read(0x0022), 0x5A);
    expect_u8(name, "SP restored", cpu.sp, 0xFD);
}

static void test_php_plp(void) {
    const char* name = "PHP/PLP";
    const uint8_t program[] = {
        0x38, /* SEC */
        0x08, /* PHP */
        0x18, /* CLC */
        0x28  /* PLP */
    };

    load_program(program, sizeof(program));
    step(2);
    expect_u8(name, "SP after PHP", cpu.sp, 0xFC);
    expect_u8(name, "pushed carry bit", mem_read(0x01FD) & 0x01, 0x01);
    step(2);
    expect_u8(name, "Carry restored by PLP", cpu_extract_sr(C), 0x01);
    expect_u8(name, "SP restored", cpu.sp, 0xFD);
}

static void test_tsx_txs(void) {
    const char* name = "TSX/TXS";
    const uint8_t program[] = {
        0xA2, 0xF0, /* LDX #$F0 */
        0x9A,       /* TXS */
        0xBA,       /* TSX */
        0x86, 0x23  /* STX $23 */
    };

    load_program(program, sizeof(program));
    step(4);
    expect_u8(name, "SP after TXS", cpu.sp, 0xF0);
    expect_u8(name, "X after TSX", cpu.x, 0xF0);
    expect_u8(name, "stored result", mem_read(0x0023), 0xF0);
}

static void run_branch_case(const char* name, const uint8_t* setup,
                            size_t setup_size, uint8_t opcode,
                            int8_t offset, uint16_t want_pc,
                            uint64_t want_branch_cycles,
                            unsigned setup_instructions,
                            uint64_t setup_cycles) {
    uint8_t program[32] = {0};
    for (size_t i = 0; i < setup_size; i++) program[i] = setup[i];
    program[setup_size] = opcode;
    program[setup_size + 1] = (uint8_t)offset;

    load_program(program, setup_size + 2);
    step(setup_instructions + 1);
    expect_u16(name, "PC", cpu.pc, want_pc);
    expect_u16(name, "branch cycles",
               (uint16_t)(cycles() - setup_cycles),
               (uint16_t)want_branch_cycles);
}

static void test_branches(void) {
    const uint8_t lda_zero[] = {0xA9, 0x00};
    const uint8_t lda_one[] = {0xA9, 0x01};
    const uint8_t lda_neg[] = {0xA9, 0x80};
    const uint8_t clc[] = {0x18, 0xEA};
    const uint8_t sec[] = {0x38, 0xEA};
    const uint8_t clv[] = {0xB8, 0xEA};
    const uint8_t set_v[] = {
        0xA9, 0x40, /* LDA #$40 */
        0x85, 0x10, /* STA $10 */
        0x24, 0x10  /* BIT $10 */
    };

    run_branch_case("BEQ taken", lda_zero, sizeof(lda_zero), 0xF0, 2, 0x8006,
                    3, 1, 2);
    run_branch_case("BEQ not taken", lda_one, sizeof(lda_one), 0xF0, 2,
                    0x8004, 2, 1, 2);
    run_branch_case("BNE taken", lda_one, sizeof(lda_one), 0xD0, 2, 0x8006,
                    3, 1, 2);
    run_branch_case("BNE not taken", lda_zero, sizeof(lda_zero), 0xD0, 2,
                    0x8004, 2, 1, 2);
    run_branch_case("BCC taken", clc, sizeof(clc), 0x90, 2, 0x8006, 3, 2, 4);
    run_branch_case("BCC not taken", sec, sizeof(sec), 0x90, 2, 0x8004, 2, 2,
                    4);
    run_branch_case("BCS taken", sec, sizeof(sec), 0xB0, 2, 0x8006, 3, 2, 4);
    run_branch_case("BCS not taken", clc, sizeof(clc), 0xB0, 2, 0x8004, 2, 2,
                    4);
    run_branch_case("BMI taken", lda_neg, sizeof(lda_neg), 0x30, 2, 0x8006,
                    3, 1, 2);
    run_branch_case("BMI not taken", lda_zero, sizeof(lda_zero), 0x30, 2,
                    0x8004, 2, 1, 2);
    run_branch_case("BPL taken", lda_zero, sizeof(lda_zero), 0x10, 2, 0x8006,
                    3, 1, 2);
    run_branch_case("BPL not taken", lda_neg, sizeof(lda_neg), 0x10, 2,
                    0x8004, 2, 1, 2);
    run_branch_case("BVC taken", clv, sizeof(clv), 0x50, 2, 0x8006, 3, 2, 4);
    run_branch_case("BVC not taken", set_v, sizeof(set_v), 0x50, 2, 0x8008,
                    2, 3, 8);
    run_branch_case("BVS taken", set_v, sizeof(set_v), 0x70, 2, 0x800A, 3, 3,
                    8);
    run_branch_case("BVS not taken", clv, sizeof(clv), 0x70, 2, 0x8004, 2, 2,
                    4);

    run_branch_case("BNE signed backward", lda_one, sizeof(lda_one), 0xD0, -2,
                    0x8002, 3, 1, 2);
}

static void test_branch_page_cross_cycles(void) {
    const char* name = "branch page-cross cycles";
    uint8_t program[0x104] = {0};
    program[0] = 0x4C;   /* JMP $80FB */
    program[1] = 0xFB;
    program[2] = 0x80;
    program[0xFB] = 0xA9; /* LDA #$00 */
    program[0xFC] = 0x00;
    program[0xFD] = 0xF0; /* BEQ +2, from PC $80FF to target $8101 */
    program[0xFE] = 0x02;

    load_program(program, sizeof(program));
    step(3);
    expect_u16(name, "PC", cpu.pc, 0x8101);
    expect_u16(name, "total cycles", (uint16_t)cycles(), 9);
}

int main(void) {
    test_jsr_rts();
    test_nested_jsr();
    test_pha_pla();
    test_php_plp();
    test_tsx_txs();
    test_branches();
    test_branch_page_cross_cycles();

    if (failures != 0) {
        fprintf(stderr, "%u CPU Batch 2 test failure(s)\n", failures);
        return 1;
    }

    printf("CPU Batch 2 tests passed\n");
    return 0;
}
