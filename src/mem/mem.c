#include "mem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "../peripherals/uart.h"

static uint8_t memory[TOTAL_MEM];

static uint8_t is_program_rom(uint16_t addr) {
    return addr >= MEM_PROGRAM_ROM_START && addr <= MEM_PROGRAM_ROM_END;
}

static uint8_t is_kernel_rom(uint16_t addr) {
    return addr >= MEM_KERNEL_ROM_START;
}

static uint8_t is_io(uint16_t addr) {
    return addr >= MEM_IO_START && addr <= MEM_IO_END;
}

static void mem_force_write(uint16_t addr, uint8_t value) {
    memory[addr] = value;
}

static void mem_clear(void) {
    memset(memory, 0, sizeof(memory));
    uart_init();
}

static uint8_t load_file_at(const char* path, uint16_t base_addr,
                            uint16_t end_addr) {
    FILE* fp = fopen(path, "rb");
    struct stat st;
    size_t fsize;
    size_t capacity;
    size_t bytes_read;

    if (fp == NULL) {
        fprintf(stderr, "[FAILED] Error while loading provided file.\n");
        return 1;
    }

    if (stat(path, &st) != 0) {
        fprintf(stderr, "[FAILED] Error while reading provided file size.\n");
        fclose(fp);
        return 1;
    }

    fsize = (size_t)st.st_size;
    capacity = (size_t)end_addr - base_addr + 1;
    if (fsize > capacity) {
        fprintf(stderr, "[FAILED] Program is too large for target memory range.\n");
        fclose(fp);
        return 1;
    }

    bytes_read = fread(memory + base_addr, 1, capacity, fp);
    fclose(fp);

    if (bytes_read != fsize) {
        fprintf(stderr,
                "[FAILED] Amount of bytes read doesn't match read file size.\n");
        return 1;
    }

    return 0;
}

uint8_t mem_load_cart(const char* path, uint16_t base_addr) {
    if (base_addr < MEM_PROGRAM_ROM_START || base_addr > MEM_PROGRAM_ROM_END) {
        fprintf(stderr, "[FAILED] Cartridge base must be in $8000-$BFFF.\n");
        return 1;
    }

    return load_file_at(path, base_addr, MEM_PROGRAM_ROM_END);
}

uint8_t mem_load_program(const char* path, uint16_t base_addr) {
    return mem_load_cart(path, base_addr);
}

uint8_t mem_load_kernel_rom(const char* path, uint16_t base_addr) {
    if (base_addr < MEM_KERNEL_ROM_START) {
        fprintf(stderr, "[FAILED] Kernel ROM base must be in $E000-$FFFF.\n");
        return 1;
    }

    return load_file_at(path, base_addr, MEM_KERNEL_ROM_END);
}

void mem_set_reset_vector(uint16_t addr) {
    /* The 6502 reset vector lives at $FFFC/$FFFD and points to the first
     * instruction. It sits in ROM, so the loader writes it directly. */
    mem_set_vector(MEM_VECTOR_RESET, addr);
}

void mem_set_vector(uint16_t vector_addr, uint16_t target_addr) {
    mem_force_write(vector_addr, target_addr & 0x00FF);
    mem_force_write(vector_addr + 1, (target_addr >> 8) & 0x00FF);
}

uint8_t mem_read(uint16_t addr) {
    if (addr == MEM_UART_DATA) {
        return uart_read_data();
    }
    if (addr == MEM_UART_STATUS) {
        return uart_read_status();
    }
    if (is_io(addr)) {
        return 0;
    }

    return memory[addr];
}

void mem_write(uint16_t addr, uint8_t value) {
    if (addr == MEM_UART_DATA) {
        uart_write_data(value);
        return;
    }
    if (is_io(addr)) {
        return;
    }

    if (is_program_rom(addr) || is_kernel_rom(addr)) {
        return;
    }

    memory[addr] = value;
}

uint16_t mem_read16(uint16_t addr) {
    uint16_t low = mem_read(addr);
    uint16_t high = mem_read(addr + 1);

    return (high << 8) | low;
}

const uint8_t* mem_region_ptr(uint16_t base_addr) {
    return memory + base_addr;
}

void mem_init_machine(const char* rom_path, const char* cart_path) {
    mem_clear();

    if (cart_path != NULL &&
        mem_load_cart(cart_path, MEM_PROGRAM_ROM_START) != 0) {
        exit(1);
    }

    if (rom_path != NULL &&
        mem_load_kernel_rom(rom_path, MEM_KERNEL_ROM_START) != 0) {
        exit(1);
    }

    mem_set_reset_vector(rom_path != NULL ? MEM_KERNEL_ROM_START
                                          : MEM_PROGRAM_ROM_START);
}

void mem_init(char* filename) {
    if (filename == NULL) {
        fprintf(stderr, "[FAILED] No cartridge image was provided.\n");
        exit(1);
    }

    mem_init_machine(NULL, filename);
}

void mem_init_monitor(const char* kernel_filename, const char* cart_filename) {
    mem_init_machine(kernel_filename, cart_filename);
}

void mem_init_kernel(const char* kernel_filename) {
    mem_init_machine(kernel_filename, NULL);
}

uint8_t mem_read_u8(uint16_t addr) {
    return mem_read(addr);
}

void mem_write_u8(uint16_t addr, uint8_t value) {
    mem_write(addr, value);
}

uint16_t mem_read_u16(uint16_t addr) {
    return mem_read16(addr);
}

int mem_dump(void) {
    FILE* fp = fopen("dump.bin", "wb+");
    size_t wb;

    if (fp == NULL) return 1;

    wb = fwrite(memory, 1, sizeof(memory), fp);
    if (wb != sizeof(memory)) {
        printf("[FAILED] Errors while dumping memory.\n");
        fclose(fp);
        return 1;
    }

    fclose(fp);
    return 0;
}
