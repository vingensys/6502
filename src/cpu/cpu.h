#ifndef INC_6502_CPU_H
#define INC_6502_CPU_H

#include <stdint.h>

struct central_processing_unit {
    uint16_t pc;
    uint8_t sp;
    uint8_t ac;
    uint8_t x;
    uint8_t y;

    /*
     * Status Register:
     *
     * bit 0: Carry
     * bit 1: Zero
     * bit 2: Interrupt
     * bit 3: Decimal
     * bit 4: Break
     * bit 5: unused, reads/pushes as 1
     * bit 6: Overflow (V)
     * bit 7: Negative
     * */
    uint8_t sr;
};

#define C 0
#define Z 1
#define I 2
#define D 3
#define B 4
#define V 6
#define N 7

extern struct central_processing_unit cpu;

struct cpu_memory_trace {
    uint16_t last_instruction_fetch_addr;
    uint8_t last_opcode_byte;
    uint8_t has_instruction_fetch;

    uint16_t last_data_read_addr;
    uint8_t last_data_read_value;
    uint8_t has_data_read;

    uint16_t last_data_write_addr;
    uint8_t last_data_write_value;
    uint8_t has_data_write;
};

extern struct cpu_memory_trace cpu_trace;

struct cpu_execution_stats {
    uint64_t total_instructions_executed;
    uint64_t total_cycles_executed;
};

void cpu_reset(void);
uint8_t cpu_extract_sr(uint8_t flag);
uint8_t cpu_mod_sr(uint8_t flag, uint8_t val);
uint8_t cpu_fetch(uint16_t addr);
uint8_t cpu_write(uint16_t addr, uint8_t data);
void cpu_exec(void);
void cpu_init(void);
void cpu_request_irq(void);
void cpu_request_nmi(void);
uint8_t cpu_status_for_push(uint8_t break_flag);
void cpu_restore_status(uint8_t status);
void cpu_push_u8(uint8_t value);
uint8_t cpu_pull_u8(void);
void cpu_service_brk(void);
struct cpu_execution_stats cpu_get_execution_stats(void);

#endif
