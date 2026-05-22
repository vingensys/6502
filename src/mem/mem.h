#ifndef INC_6502_MEM_H
#define INC_6502_MEM_H

#include <stddef.h>
#include <stdint.h>

#define TOTAL_MEM (1024 * 64)

#define MEM_ZERO_PAGE_START 0x0000
#define MEM_ZERO_PAGE_END 0x00FF
#define MEM_STACK_START 0x0100
#define MEM_STACK_END 0x01FF
#define MEM_RAM_START 0x0200
#define MEM_RAM_END 0x7FFF
#define MEM_PROGRAM_ROM_START 0x8000
#define MEM_PROGRAM_ROM_END 0xBFFF
#define MEM_EXPANSION_RAM_START 0xC000
#define MEM_EXPANSION_RAM_END 0xCFFF
#define MEM_IO_START 0xD000
#define MEM_IO_END 0xDFFF
#define MEM_UART_DATA 0xD010
#define MEM_UART_STATUS 0xD011
#define MEM_KERNEL_ROM_START 0xE000
#define MEM_KERNEL_ROM_END 0xFFFF

#define MEM_VECTOR_NMI 0xFFFA
#define MEM_VECTOR_RESET 0xFFFC
#define MEM_VECTOR_IRQ 0xFFFE

void mem_init(char* filename);
void mem_init_machine(const char* rom_path, const char* cart_path);
void mem_init_monitor(const char* kernel_filename, const char* cart_filename);
void mem_init_kernel(const char* kernel_filename);
uint8_t mem_read(uint16_t addr);
void mem_write(uint16_t addr, uint8_t value);
uint16_t mem_read16(uint16_t addr);
uint8_t mem_load_program(const char* path, uint16_t base_addr);
uint8_t mem_load_cart(const char* path, uint16_t base_addr);
void mem_set_reset_vector(uint16_t addr);
void mem_set_vector(uint16_t vector_addr, uint16_t target_addr);
uint8_t mem_load_kernel_rom(const char* path, uint16_t base_addr);
const uint8_t* mem_region_ptr(uint16_t base_addr);
int mem_dump(void);

/* Compatibility aliases while the rest of the emulator settles on the bus API. */
uint8_t mem_read_u8(uint16_t addr);
void mem_write_u8(uint16_t addr, uint8_t value);
uint16_t mem_read_u16(uint16_t addr);

#endif
