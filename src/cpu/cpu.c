#include "cpu.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../mem/mem.h"
#include "../utils/misc.h"
#include "instructions.h"

/**
 * Little-endian 8-bit microprocessor that expects addresses
 * to be store in memory least significant byte first
 * */
struct central_processing_unit cpu;
struct cpu_memory_trace cpu_trace;
static struct cpu_execution_stats cpu_stats;
static uint8_t irq_pending = 0;
static uint8_t nmi_pending = 0;

// clock cycles, every fetch implies a clock cycle
uint32_t cycles = 0;

/**
 * cpu_init: Initialize CPU by linking it to the memory
 * @param void
 * @return void
 */
void cpu_init(void) {}

/**
 * cpu_extract_sr: Extract one of the 7 flags from the status reg.
 * @param flag The flag to be extracted
 * @return the bit of the wanted flag
 * */
uint8_t cpu_extract_sr(uint8_t flag) { return ((cpu.sr >> (flag % 8)) & 1); }

/**
 * cpu_mod_sr: Modify the sr register (flags)
 * @param flag The flag to set
 * @param val The value
 * @return 0 if success, 1 if failure
 */
uint8_t cpu_mod_sr(uint8_t flag, uint8_t val) {
    if (val != 0 && val != 1) return 1;

    if (flag < 8 && flag != 5) {
        if (val == 1) {
            SET_BIT(cpu.sr, flag);
        } else {
            CLEAR_BIT(cpu.sr, flag);
        }
        return 0;
    } else {
        return 1;
    }
}

uint8_t cpu_status_for_push(uint8_t break_flag) {
    uint8_t status = cpu.sr | (1 << 5);

    if (break_flag) {
        status |= (1 << B);
    } else {
        status &= ~(1 << B);
    }

    return status;
}

void cpu_restore_status(uint8_t status) {
    cpu.sr = (status | (1 << 5)) & ~(1 << B);
}

void cpu_push_u8(uint8_t value) {
    cpu_write(0x0100 + cpu.sp, value);
    cpu.sp--;
}

uint8_t cpu_pull_u8(void) {
    cpu.sp++;
    return cpu_fetch(0x0100 + cpu.sp);
}

static void cpu_service_interrupt(uint16_t vector_addr, uint8_t break_flag) {
    cpu_push_u8((cpu.pc >> 8) & 0x00FF);
    cpu_push_u8(cpu.pc & 0x00FF);
    cpu_push_u8(cpu_status_for_push(break_flag));

    cpu_mod_sr(I, 1);
    cpu_mod_sr(B, 0);
    cpu.sr |= (1 << 5);
    cpu.pc = mem_read16(vector_addr);
}

void cpu_service_brk(void) {
    cpu_service_interrupt(MEM_VECTOR_IRQ, 1);
}

void cpu_request_irq(void) {
    irq_pending = 1;
}

void cpu_request_nmi(void) {
    nmi_pending = 1;
}

static uint8_t cpu_service_pending_interrupt(uint32_t* instruction_cycles) {
    if (nmi_pending) {
        nmi_pending = 0;
        cpu_service_interrupt(MEM_VECTOR_NMI, 0);
        *instruction_cycles = 7;
        return 1;
    }

    if (irq_pending) {
        irq_pending = 0;
        if (cpu_extract_sr(I)) return 0;

        cpu_service_interrupt(MEM_VECTOR_IRQ, 0);
        *instruction_cycles = 7;
        return 1;
    }

    return 0;
}

/**
 * cpu_reset: Reset the CPU to its initial state. Wrapper around reset()
 *
 * @param void
 * @return void
 * */
void cpu_reset(void) {
    reset();

    cpu.pc = mem_read16(MEM_VECTOR_RESET);
    debug_print("(cpu_reset) reset vector PC: 0x%X\n", cpu.pc);

    cycles = 0;
    cpu_stats.total_instructions_executed = 0;
    cpu_stats.total_cycles_executed = 0;
    cpu_trace.has_instruction_fetch = 0;
    cpu_trace.has_data_read = 0;
    cpu_trace.has_data_write = 0;
    irq_pending = 0;
    nmi_pending = 0;
}

/**
 * cpu_fetch: Fetch memory from a given address
 * @param void
 * @return void
 */
uint8_t cpu_fetch(uint16_t addr) {
    debug_print("(cpu_fetch) reading at: 0x%X\n", addr);
    uint8_t data = mem_read(addr);
    debug_print("(cpu_fetch) GOT: 0x%X\n", data);

    if (addr != cpu.pc) {
        cpu_trace.last_data_read_addr = addr;
        cpu_trace.last_data_read_value = data;
        cpu_trace.has_data_read = 1;
    }

    if (addr == cpu.pc) cpu.pc++;

    return data;
}

/**
 * cpu_write: Wrapper for write_mem()
 * @param addr The address to be written to
 * @param data The data to be written
 * @return 0 if success, 1 if failure
 */
uint8_t cpu_write(uint16_t addr, uint8_t data) {
    cpu_trace.last_data_write_addr = addr;
    cpu_trace.last_data_write_value = data;
    cpu_trace.has_data_write = 1;

    mem_write(addr, data);
    return 0;
}

/**
 * cpu_exec: Execute fetched data (single stepping)
 * @param void
 * @return void
 */
void cpu_exec(void) {
    debug_print("(cpu_exec) cycles: %d\n", cycles);

    int8_t fetched;
    uint32_t instruction_cycles = 0;
    do {
        debug_print("(loop) cycles: %d\n", cycles);
        // executing in a take
        if (cycles == 0) {
            if (cpu_service_pending_interrupt(&instruction_cycles)) {
                cycles = instruction_cycles;
            } else {
                uint16_t opcode_addr = cpu.pc;
                fetched = cpu_fetch(cpu.pc);
                cpu_trace.last_instruction_fetch_addr = opcode_addr;
                cpu_trace.last_opcode_byte = fetched;
                cpu_trace.has_instruction_fetch = 1;

                debug_print("(cpu_exec) fetched: 0x%X\n", fetched);
                inst_exec(fetched, &cycles);
                instruction_cycles = cycles;
            }
        }
        cycles--;
    } while (cycles != 0);

    cpu_stats.total_instructions_executed++;
    cpu_stats.total_cycles_executed += instruction_cycles;
}

struct cpu_execution_stats cpu_get_execution_stats(void) {
    return cpu_stats;
}
