#define _POSIX_C_SOURCE 200809L

#include "interface.h"

#include <ncurses.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../cpu/cpu.h"
#include "../mem/mem.h"
#include "kinput.h"
#include "uart.h"

#define MAX_BYTES_PER_ROW 16
#define PANE_GAP 1
#define PROGRAM_BASE MEM_PROGRAM_ROM_START
#define PROGRAM_SIZE (MEM_PROGRAM_ROM_END - MEM_PROGRAM_ROM_START + 1)
#define MIN_TERM_ROWS WIN_ROWS
#define MIN_TERM_COLS WIN_COLS

#define PAIR_TITLE 1
#define PAIR_ACTIVE 2
#define PAIR_PC 3
#define PAIR_READ 4
#define PAIR_WRITE 5

enum memory_view {
    MEM_VIEW_ZERO_PAGE,
    MEM_VIEW_STACK,
    MEM_VIEW_PROGRAM,
    MEM_VIEW_KERNEL_ROM,
    MEM_VIEW_COUNT
};

struct memory_region {
    const char* title;
    uint16_t base_addr;
    uint16_t size;
    const uint8_t* data;
};

struct interface_state {
    enum memory_view active_view;
    uint16_t scroll[MEM_VIEW_COUNT];
    enum ui_mode mode;
};

struct opcode_decode {
    uint8_t opcode;
    const char* mnemonic;
    const char* mode;
    uint8_t length;
};

struct interface_layout {
    int rows;
    int cols;
    int too_small;
    WINDOW* header;
    WINDOW* registers;
    WINDOW* flags;
    WINDOW* fetch;
    WINDOW* vectors;
    WINDOW* trace;
    WINDOW* performance;
    WINDOW* memory[MEM_VIEW_COUNT];
    WINDOW* terminal;
    WINDOW* uart_terminal;
    WINDOW* footer;
};

static struct interface_state ui_state = {
    MEM_VIEW_PROGRAM, {0, 0, 0}, DEBUGGER_MODE};
static struct interface_layout layout = {0};
static uint8_t colors_ready = 0;

struct performance_sample {
    uint8_t initialized;
    struct timespec last_time;
    uint64_t last_total_instructions;
    uint64_t last_total_cycles;
    double last_ips;
    double last_cycles_per_second;
};

static struct performance_sample perf_sample = {0};

static const struct opcode_decode decode_table[] = {
    {0x00, "BRK", "imp", 1},  {0x18, "CLC", "imp", 1},
    {0x29, "AND", "imm", 2},  {0x2D, "AND", "abs", 3},
    {0x4C, "JMP", "abs", 3},  {0x6C, "JMP", "ind", 3},
    {0x61, "ADC", "izx", 2},  {0x65, "ADC", "zp", 2},
    {0x69, "ADC", "imm", 2},  {0x6D, "ADC", "abs", 3},
    {0x71, "ADC", "izy", 2},  {0x75, "ADC", "zpx", 2},
    {0x79, "ADC", "absy", 3}, {0x7D, "ADC", "absx", 3},
    {0x81, "STA", "izx", 2},  {0x85, "STA", "zp", 2},
    {0x86, "STX", "zp", 2},   {0x8D, "STA", "abs", 3},
    {0x8E, "STX", "abs", 3},  {0x91, "STA", "izy", 2},
    {0x95, "STA", "zpx", 2},  {0x96, "STX", "zpy", 2},
    {0x99, "STA", "absy", 3}, {0x9D, "STA", "absx", 3},
    {0xA0, "LDY", "imm", 2},  {0xA1, "LDA", "izx", 2},
    {0xA2, "LDX", "imm", 2},  {0xA4, "LDY", "zp", 2},
    {0xA5, "LDA", "zp", 2},   {0xA6, "LDX", "zp", 2},
    {0xA9, "LDA", "imm", 2},  {0xAC, "LDY", "abs", 3},
    {0xAD, "LDA", "abs", 3},  {0xAE, "LDX", "abs", 3},
    {0xB1, "LDA", "izy", 2},  {0xB4, "LDY", "zpx", 2},
    {0xB5, "LDA", "zpx", 2},  {0xB6, "LDX", "zpy", 2},
    {0xB9, "LDA", "absy", 3}, {0xBC, "LDY", "absx", 3},
    {0xBD, "LDA", "absx", 3}, {0xBE, "LDX", "absy", 3},
    {0xC8, "INY", "imp", 1},  {0xCA, "DEX", "imp", 1},
    {0xD0, "BNE", "rel", 2},  {0xE8, "INX", "imp", 1},
    {0x88, "DEY", "imp", 1},  {0xEA, "NOP", "imp", 1},
    {0xF0, "BEQ", "rel", 2},
};

static void ensure_colors(void) {
    if (colors_ready) return;

    colors_ready = 1;
    if (!has_colors()) return;

    start_color();
    use_default_colors();
    init_pair(PAIR_TITLE, COLOR_CYAN, -1);
    init_pair(PAIR_ACTIVE, COLOR_BLACK, COLOR_CYAN);
    init_pair(PAIR_PC, COLOR_BLACK, COLOR_YELLOW);
    init_pair(PAIR_READ, COLOR_BLACK, COLOR_GREEN);
    init_pair(PAIR_WRITE, COLOR_BLACK, COLOR_RED);
}

static int color_attr(int pair, int fallback) {
    if (!has_colors()) return fallback;
    return COLOR_PAIR(pair) | fallback;
}

static void screen_size(int* rows, int* cols) {
    getmaxyx(stdscr, *rows, *cols);
}

static int top_height(int rows) {
    if (rows < 30) return 12;
    return 14;
}

static int footer_height(void) {
    return 3;
}

static int terminal_height(void) {
    return 6;
}

static int memory_top(int rows) {
    (void)rows;
    return top_height(rows);
}

static int memory_height(int rows) {
    int height = rows - top_height(rows) - terminal_height() - footer_height();
    return height < 8 ? 8 : height;
}

static int pane_width(int cols) {
    return (cols - (PANE_GAP * (MEM_VIEW_COUNT - 1))) / MEM_VIEW_COUNT;
}

static int pane_left(enum memory_view view, int cols) {
    return view * (pane_width(cols) + PANE_GAP);
}

static int bytes_per_row_for_width(int width) {
    int bytes = (width - 8) / 3;

    if (bytes > MAX_BYTES_PER_ROW) return MAX_BYTES_PER_ROW;
    if (bytes < 4) return 4;
    return bytes;
}

static uint8_t read_addr(uint16_t addr) {
    return mem_read(addr);
}

static void get_region(enum memory_view view, struct memory_region* region) {
    switch (view) {
        case MEM_VIEW_ZERO_PAGE:
            region->title = "Zero Page";
            region->base_addr = MEM_ZERO_PAGE_START;
            region->size = MEM_ZERO_PAGE_END - MEM_ZERO_PAGE_START + 1;
            region->data = mem_region_ptr(MEM_ZERO_PAGE_START);
            break;

        case MEM_VIEW_STACK:
            region->title = "Stack";
            region->base_addr = MEM_STACK_START;
            region->size = MEM_STACK_END - MEM_STACK_START + 1;
            region->data = mem_region_ptr(MEM_STACK_START);
            break;

        case MEM_VIEW_PROGRAM:
            region->title = "Program Data";
            region->base_addr = PROGRAM_BASE;
            region->size = PROGRAM_SIZE;
            region->data = mem_region_ptr(PROGRAM_BASE);
            break;

        case MEM_VIEW_KERNEL_ROM:
        default:
            region->title = "Kernel ROM";
            region->base_addr = MEM_KERNEL_ROM_START;
            region->size = MEM_KERNEL_ROM_END - MEM_KERNEL_ROM_START + 1;
            region->data = mem_region_ptr(MEM_KERNEL_ROM_START);
            break;
    }
}

static void draw_box(WINDOW* win, const char* title, int active) {
    werase(win);
    if (active) wattron(win, color_attr(PAIR_ACTIVE, A_BOLD));
    box(win, 0, 0);
    if (title != NULL) {
        mvwprintw(win, 0, 2, " %s ", title);
    }
    if (active) wattroff(win, color_attr(PAIR_ACTIVE, A_BOLD));
}

static uint16_t pane_data_rows_for_height(int height) {
    return height > 5 ? height - 5 : 1;
}

static void delete_window(WINDOW** win) {
    if (*win != NULL) {
        delwin(*win);
        *win = NULL;
    }
}

static void delete_layout(void) {
    delete_window(&layout.header);
    delete_window(&layout.registers);
    delete_window(&layout.flags);
    delete_window(&layout.fetch);
    delete_window(&layout.vectors);
    delete_window(&layout.trace);
    delete_window(&layout.performance);
    for (enum memory_view view = MEM_VIEW_ZERO_PAGE; view < MEM_VIEW_COUNT;
         view++) {
        delete_window(&layout.memory[view]);
    }
    delete_window(&layout.terminal);
    delete_window(&layout.uart_terminal);
    delete_window(&layout.footer);
}

static int create_layout(void) {
    int rows, cols;
    int y = 3;
    int h;
    int x;
    int w_reg;
    int w_flags;
    int w_fetch;
    int w_vectors;
    int w_trace;
    int trace_h;
    int mem_y;
    int mem_h;
    int mem_w;

    screen_size(&rows, &cols);
    if (rows == layout.rows && cols == layout.cols && layout.header != NULL) {
        return !layout.too_small;
    }

    delete_layout();
    layout.rows = rows;
    layout.cols = cols;
    layout.too_small = rows < MIN_TERM_ROWS || cols < MIN_TERM_COLS;
    werase(stdscr);

    if (layout.too_small) {
        return 0;
    }

    layout.header = newwin(3, cols, 0, 0);
    layout.uart_terminal = newwin(rows, cols, 0, 0);

    h = top_height(rows) - y;
    w_reg = 26;
    w_flags = 24;
    w_vectors = 34;
    w_trace = 34;
    w_fetch = cols - 2 - w_reg - w_flags - w_vectors - w_trace - 4;
    if (w_fetch < 32) w_fetch = 32;

    x = 1;
    layout.registers = newwin(h, w_reg, y, x);
    x += w_reg + 1;
    layout.flags = newwin(h, w_flags, y, x);
    x += w_flags + 1;
    layout.fetch = newwin(h, w_fetch, y, x);
    x += w_fetch + 1;
    layout.vectors = newwin(h, w_vectors, y, x);
    x += w_vectors + 1;
    w_trace = cols - x - 1;
    trace_h = 5;
    if (trace_h > h - 3) trace_h = h - 3;
    layout.trace = newwin(trace_h, w_trace, y, x);
    layout.performance = newwin(h - trace_h, w_trace, y + trace_h, x);

    mem_y = memory_top(rows);
    mem_h = memory_height(rows);
    mem_w = pane_width(cols);
    for (enum memory_view view = MEM_VIEW_ZERO_PAGE; view < MEM_VIEW_COUNT;
         view++) {
        layout.memory[view] =
            newwin(mem_h, mem_w, mem_y, pane_left(view, cols));
    }

    layout.terminal =
        newwin(terminal_height(), cols, rows - footer_height() -
                                           terminal_height(), 0);
    layout.footer = newwin(footer_height(), cols, rows - footer_height(), 0);

    return layout.header != NULL && layout.registers != NULL &&
           layout.flags != NULL && layout.fetch != NULL &&
           layout.vectors != NULL && layout.trace != NULL &&
           layout.performance != NULL &&
           layout.memory[MEM_VIEW_ZERO_PAGE] != NULL &&
           layout.memory[MEM_VIEW_STACK] != NULL &&
           layout.memory[MEM_VIEW_PROGRAM] != NULL &&
           layout.memory[MEM_VIEW_KERNEL_ROM] != NULL &&
           layout.terminal != NULL && layout.uart_terminal != NULL &&
           layout.footer != NULL;
}

static int layout_ready(void) {
    create_layout();
    return !layout.too_small && layout.header != NULL;
}

static void display_too_small(void) {
    int rows, cols;

    screen_size(&rows, &cols);
    werase(stdscr);
    mvprintw(1, 2, "Terminal too small. Need at least %dx%d. Current: %dx%d.",
             MIN_TERM_COLS, MIN_TERM_ROWS, cols, rows);
    mvprintw(3, 2, "Resize the SSH terminal, then press any key.");
    wnoutrefresh(stdscr);
    doupdate();
}

static uint16_t max_scroll_for_view(enum memory_view view) {
    struct memory_region region;
    int rows, cols;
    uint16_t data_rows;
    uint16_t visible_bytes;

    get_region(view, &region);
    screen_size(&rows, &cols);
    data_rows = pane_data_rows_for_height(memory_height(rows));
    visible_bytes = data_rows * bytes_per_row_for_width(pane_width(cols));

    if (region.size <= visible_bytes) return 0;
    return ((region.size - visible_bytes) / MAX_BYTES_PER_ROW) *
           MAX_BYTES_PER_ROW;
}

static void clamp_scroll(enum memory_view view) {
    uint16_t max_scroll;

    max_scroll = max_scroll_for_view(view);
    if (ui_state.scroll[view] > max_scroll) ui_state.scroll[view] = max_scroll;
}

static const struct opcode_decode* find_decode(uint8_t opcode) {
    size_t count = sizeof(decode_table) / sizeof(decode_table[0]);

    for (size_t i = 0; i < count; i++) {
        if (decode_table[i].opcode == opcode) return &decode_table[i];
    }

    return NULL;
}

static void format_decode(uint16_t pc, char* buffer, size_t size) {
    uint8_t opcode = read_addr(pc);
    uint8_t b1 = read_addr(pc + 1);
    uint8_t b2 = read_addr(pc + 2);
    const struct opcode_decode* decoded = find_decode(opcode);

    if (decoded == NULL) {
        snprintf(buffer, size, "Unknown");
    } else if (strcmp(decoded->mode, "imm") == 0) {
        snprintf(buffer, size, "%s #$%02X", decoded->mnemonic, b1);
    } else if (strcmp(decoded->mode, "abs") == 0) {
        snprintf(buffer, size, "%s $%04X", decoded->mnemonic,
                 (uint16_t)((b2 << 8) | b1));
    } else if (strcmp(decoded->mode, "absx") == 0) {
        snprintf(buffer, size, "%s $%04X,X", decoded->mnemonic,
                 (uint16_t)((b2 << 8) | b1));
    } else if (strcmp(decoded->mode, "absy") == 0) {
        snprintf(buffer, size, "%s $%04X,Y", decoded->mnemonic,
                 (uint16_t)((b2 << 8) | b1));
    } else if (strcmp(decoded->mode, "zp") == 0) {
        snprintf(buffer, size, "%s $%02X", decoded->mnemonic, b1);
    } else if (strcmp(decoded->mode, "zpx") == 0) {
        snprintf(buffer, size, "%s $%02X,X", decoded->mnemonic, b1);
    } else if (strcmp(decoded->mode, "zpy") == 0) {
        snprintf(buffer, size, "%s $%02X,Y", decoded->mnemonic, b1);
    } else if (strcmp(decoded->mode, "ind") == 0) {
        snprintf(buffer, size, "%s ($%04X)", decoded->mnemonic,
                 (uint16_t)((b2 << 8) | b1));
    } else if (strcmp(decoded->mode, "izx") == 0) {
        snprintf(buffer, size, "%s ($%02X,X)", decoded->mnemonic, b1);
    } else if (strcmp(decoded->mode, "izy") == 0) {
        snprintf(buffer, size, "%s ($%02X),Y", decoded->mnemonic, b1);
    } else if (strcmp(decoded->mode, "rel") == 0) {
        snprintf(buffer, size, "%s $%04X", decoded->mnemonic,
                 (uint16_t)(pc + 2 + (int8_t)b1));
    } else {
        snprintf(buffer, size, "%s", decoded->mnemonic);
    }
}

static void format_bytes(uint16_t pc, char* buffer, size_t size) {
    uint8_t opcode = read_addr(pc);
    const struct opcode_decode* decoded = find_decode(opcode);
    uint8_t count = decoded == NULL ? 4 : decoded->length;

    buffer[0] = '\0';
    for (uint8_t i = 0; i < count; i++) {
        char next_byte[5];

        snprintf(next_byte, sizeof(next_byte), "%s%02X", i == 0 ? "" : " ",
                 read_addr(pc + i));
        strncat(buffer, next_byte, size - strlen(buffer) - 1);
    }
}

static double elapsed_seconds(struct timespec start, struct timespec end) {
    return (double)(end.tv_sec - start.tv_sec) +
           ((double)(end.tv_nsec - start.tv_nsec) / 1000000000.0);
}

static void format_rate(double value, const char* suffix, char* buffer,
                        size_t size) {
    if (value >= 1000000.0) {
        snprintf(buffer, size, "%.2fM%s", value / 1000000.0, suffix);
    } else if (value >= 1000.0) {
        snprintf(buffer, size, "%.1fK%s", value / 1000.0, suffix);
    } else {
        snprintf(buffer, size, "%.0f%s", value, suffix);
    }
}

static void format_frequency(double hz, char* buffer, size_t size) {
    if (hz >= 1000000.0) {
        snprintf(buffer, size, "%.2f MHz", hz / 1000000.0);
    } else if (hz >= 1000.0) {
        snprintf(buffer, size, "%.1f kHz", hz / 1000.0);
    } else {
        snprintf(buffer, size, "%.0f Hz", hz);
    }
}

static void update_performance_sample(void) {
    struct cpu_execution_stats stats = cpu_get_execution_stats();
    struct timespec now;
    double elapsed;

    clock_gettime(CLOCK_MONOTONIC, &now);
    if (!perf_sample.initialized) {
        perf_sample.initialized = 1;
        perf_sample.last_time = now;
        perf_sample.last_total_instructions = stats.total_instructions_executed;
        perf_sample.last_total_cycles = stats.total_cycles_executed;
        return;
    }
    if (stats.total_instructions_executed < perf_sample.last_total_instructions ||
        stats.total_cycles_executed < perf_sample.last_total_cycles) {
        perf_sample.last_time = now;
        perf_sample.last_total_instructions = stats.total_instructions_executed;
        perf_sample.last_total_cycles = stats.total_cycles_executed;
        perf_sample.last_ips = 0.0;
        perf_sample.last_cycles_per_second = 0.0;
        return;
    }

    elapsed = elapsed_seconds(perf_sample.last_time, now);
    if (elapsed < 0.25) return;

    perf_sample.last_ips =
        (double)(stats.total_instructions_executed -
                 perf_sample.last_total_instructions) /
        elapsed;
    perf_sample.last_cycles_per_second =
        (double)(stats.total_cycles_executed - perf_sample.last_total_cycles) /
        elapsed;
    perf_sample.last_time = now;
    perf_sample.last_total_instructions = stats.total_instructions_executed;
    perf_sample.last_total_cycles = stats.total_cycles_executed;
}

static void display_header_window(void) {
    WINDOW* win = layout.header;
    int width;

    if (win == NULL) return;
    width = getmaxx(win);

    draw_box(win, NULL, 0);
    wattron(win, color_attr(PAIR_TITLE, A_BOLD));
    mvwprintw(win, 1, 2, "6502 Emulator Debugger");
    wattroff(win, color_attr(PAIR_TITLE, A_BOLD));
    mvwprintw(win, 1, width - 32, "mode: %s",
              cpu_trace.has_instruction_fetch ? "debug" : "ready");
    wnoutrefresh(win);
}

static void display_register_window(void) {
    WINDOW* win = layout.registers;

    if (win == NULL) return;

    draw_box(win, "Registers", 0);
    mvwprintw(win, 1, 2, "A   %02X", cpu.ac);
    mvwprintw(win, 2, 2, "X   %02X", cpu.x);
    mvwprintw(win, 3, 2, "Y   %02X", cpu.y);
    mvwprintw(win, 1, 12, "PC  %04X", cpu.pc);
    mvwprintw(win, 2, 12, "SP  %02X", cpu.sp);
    mvwprintw(win, 3, 12, "SR  %02X", cpu.sr);
    wnoutrefresh(win);
}

static void display_flags_window(void) {
    WINDOW* win = layout.flags;

    if (win == NULL) return;

    draw_box(win, "Flags", 0);
    mvwprintw(win, 1, 2, "N V - B D I Z C");
    mvwprintw(win, 2, 2, "%u %u 0 %u %u %u %u %u", cpu_extract_sr(N),
              cpu_extract_sr(V), cpu_extract_sr(B), cpu_extract_sr(D),
              cpu_extract_sr(I), cpu_extract_sr(Z), cpu_extract_sr(C));
    wnoutrefresh(win);
}

static void display_fetch_window(void) {
    WINDOW* win = layout.fetch;
    char decoded[40];
    char bytes_text[20];

    if (win == NULL) return;

    draw_box(win, "Fetch / Decode", 0);
    format_decode(cpu.pc, decoded, sizeof(decoded));
    format_bytes(cpu.pc, bytes_text, sizeof(bytes_text));

    mvwprintw(win, 1, 2, "PC      $%04X", cpu.pc);
    mvwprintw(win, 2, 2, "Opcode  %02X", read_addr(cpu.pc));
    mvwprintw(win, 3, 2, "Bytes   %s", bytes_text);
    mvwprintw(win, 4, 2, "Decode  %s", decoded);
    wnoutrefresh(win);
}

static void display_vectors_window(void) {
    WINDOW* win = layout.vectors;

    if (win == NULL) return;

    draw_box(win, "Vectors", 0);
    mvwprintw(win, 1, 2, "NMI   $FFFA/$FFFB -> $%04X",
              mem_read16(MEM_VECTOR_NMI));
    wattron(win, color_attr(PAIR_PC, A_BOLD));
    mvwprintw(win, 2, 2, "RESET $FFFC/$FFFD -> $%04X",
              mem_read16(MEM_VECTOR_RESET));
    wattroff(win, color_attr(PAIR_PC, A_BOLD));
    mvwprintw(win, 3, 2, "IRQ   $FFFE/$FFFF -> $%04X",
              mem_read16(MEM_VECTOR_IRQ));
    wnoutrefresh(win);
}

static void display_trace_window(void) {
    WINDOW* win = layout.trace;

    if (win == NULL) return;

    draw_box(win, "Trace", 0);
    wattron(win, color_attr(PAIR_PC, A_BOLD));
    mvwprintw(win, 1, 2, "FETCH");
    wattroff(win, color_attr(PAIR_PC, A_BOLD));
    if (cpu_trace.has_instruction_fetch) {
        mvwprintw(win, 1, 10, "$%04X -> %02X",
                  cpu_trace.last_instruction_fetch_addr,
                  cpu_trace.last_opcode_byte);
    } else {
        mvwprintw(win, 1, 10, "--");
    }

    wattron(win, color_attr(PAIR_READ, A_BOLD));
    mvwprintw(win, 2, 2, "READ ");
    wattroff(win, color_attr(PAIR_READ, A_BOLD));
    if (cpu_trace.has_data_read) {
        mvwprintw(win, 2, 10, "$%04X -> %02X", cpu_trace.last_data_read_addr,
                  cpu_trace.last_data_read_value);
    } else {
        mvwprintw(win, 2, 10, "--");
    }

    wattron(win, color_attr(PAIR_WRITE, A_BOLD));
    mvwprintw(win, 3, 2, "WRITE");
    wattroff(win, color_attr(PAIR_WRITE, A_BOLD));
    if (cpu_trace.has_data_write) {
        mvwprintw(win, 3, 10, "$%04X <- %02X", cpu_trace.last_data_write_addr,
                  cpu_trace.last_data_write_value);
    } else {
        mvwprintw(win, 3, 10, "--");
    }
    wnoutrefresh(win);
}

static void display_performance_window(void) {
    WINDOW* win = layout.performance;
    struct cpu_execution_stats stats = cpu_get_execution_stats();
    char ips_text[20];
    char freq_text[20];

    if (win == NULL) return;

    update_performance_sample();
    format_rate(perf_sample.last_ips, "/s", ips_text, sizeof(ips_text));
    format_frequency(perf_sample.last_cycles_per_second, freq_text,
                     sizeof(freq_text));

    draw_box(win, "Performance", 0);
    mvwprintw(win, 1, 2, "Run   %s",
              kinput_is_running() ? "RUNNING" : "PAUSED");
    mvwprintw(win, 2, 2, "Total %llu",
              (unsigned long long)stats.total_instructions_executed);
    mvwprintw(win, 3, 2, "IPS   %s", ips_text);
    if (getmaxy(win) > 6) {
        mvwprintw(win, 4, 2, "Freq  %s", freq_text);
        mvwprintw(win, 5, 2, "Target unlimited");
    } else {
        mvwprintw(win, 4, 2, "Freq  %s", freq_text);
    }
    wnoutrefresh(win);
}

static int attr_for_address(enum memory_view view, uint16_t addr) {
    uint16_t sp_addr = 0x0100 + cpu.sp;

    if (cpu_trace.has_data_write && addr == cpu_trace.last_data_write_addr) {
        return color_attr(PAIR_WRITE, A_BOLD);
    }
    if (cpu_trace.has_data_read && addr == cpu_trace.last_data_read_addr) {
        return color_attr(PAIR_READ, A_BOLD);
    }
    if ((view == MEM_VIEW_PROGRAM || view == MEM_VIEW_KERNEL_ROM) &&
        addr == cpu.pc) {
        return color_attr(PAIR_PC, A_BOLD);
    }
    if ((view == MEM_VIEW_PROGRAM || view == MEM_VIEW_KERNEL_ROM) &&
        cpu_trace.has_instruction_fetch &&
        addr == cpu_trace.last_instruction_fetch_addr) {
        return color_attr(PAIR_PC, A_REVERSE);
    }
    if (view == MEM_VIEW_STACK && addr == sp_addr) {
        return A_REVERSE | A_BOLD;
    }

    return A_NORMAL;
}

static void display_memory_pane(enum memory_view view) {
    struct memory_region region;
    WINDOW* win = layout.memory[view];
    uint16_t scroll;
    uint16_t max_scroll;
    uint16_t visible_end;
    uint32_t visible_end_raw;
    int height;
    int width;
    uint16_t data_rows;
    int bytes_per_row;
    int active = view == ui_state.active_view;

    if (win == NULL) return;

    height = getmaxy(win);
    width = getmaxx(win);
    data_rows = pane_data_rows_for_height(height);
    bytes_per_row = bytes_per_row_for_width(width);

    get_region(view, &region);
    clamp_scroll(view);
    scroll = ui_state.scroll[view];
    max_scroll = max_scroll_for_view(view);
    visible_end_raw = (uint32_t)region.base_addr + scroll +
                      (data_rows * bytes_per_row) - 1;
    if (visible_end_raw > (uint32_t)region.base_addr + region.size - 1) {
        visible_end = region.base_addr + region.size - 1;
    } else {
        visible_end = (uint16_t)visible_end_raw;
    }

    draw_box(win, region.title, active);
    mvwprintw(win, 1, 1, "$%04X-$%04X", region.base_addr + scroll,
              visible_end);
    mvwprintw(win, 1, width - 17, "bottom $%04X", region.base_addr + max_scroll);

    for (uint16_t row = 0; row < data_rows; row++) {
        uint16_t offset = scroll + (row * bytes_per_row);
        uint16_t addr = region.base_addr + offset;
        int row_y = 3 + row;

        if (offset >= region.size) {
            continue;
        }

        mvwprintw(win, row_y, 1, "%04X:", addr);

        for (uint16_t col = 0; col < bytes_per_row; col++) {
            uint16_t byte_offset = offset + col;
            uint16_t byte_addr = region.base_addr + byte_offset;
            int attr;

            if (byte_offset >= region.size) break;

            attr = attr_for_address(view, byte_addr);
            if (attr != A_NORMAL) wattron(win, attr);
            mvwprintw(win, row_y, 7 + (col * 3), "%02X",
                      region.data[byte_offset]);
            if (attr != A_NORMAL) wattroff(win, attr);
        }
    }

    wnoutrefresh(win);
}

static void display_terminal_window(void) {
    WINDOW* win = layout.terminal;
    const char* buffer = uart_get_buffer();
    size_t len = uart_get_buffer_len();
    int width;
    int height;
    int content_w;
    int content_h;
    int y = 1;
    int x = 1;
    size_t start = 0;

    if (win == NULL) return;

    height = getmaxy(win);
    width = getmaxx(win);
    content_w = width - 2;
    content_h = height - 2;

    draw_box(win, "Terminal", 0);
    mvwprintw(win, 0, width - 12, " RX %03u ",
              (unsigned int)uart_get_rx_count());

    if (content_w <= 0 || content_h <= 0) {
        wnoutrefresh(win);
        return;
    }

    if (len > (size_t)(content_w * content_h)) {
        start = len - (size_t)(content_w * content_h);
    }

    for (size_t i = start; i < len; i++) {
        char ch = buffer[i];

        if (ch == '\n') {
            y++;
            x = 1;
        } else {
            mvwaddch(win, y, x, ch);
            x++;
            if (x > content_w) {
                y++;
                x = 1;
            }
        }

        if (y > content_h) break;
    }

    wnoutrefresh(win);
}

void interface_display_uart_terminal(void) {
    WINDOW* win;
    const char* buffer;
    size_t len;
    int rows;
    int cols;
    int content_h;
    int content_w;
    int y = 1;
    int x = 0;
    size_t start = 0;
    uint8_t status;

    ensure_colors();
    if (!layout_ready()) {
        display_too_small();
        return;
    }

    win = layout.uart_terminal;
    if (win == NULL) return;

    buffer = uart_get_buffer();
    len = uart_get_buffer_len();
    rows = getmaxy(win);
    cols = getmaxx(win);
    content_h = rows - 1;
    content_w = cols;
    status = uart_read_status();

    werase(win);
    wattron(win, color_attr(PAIR_TITLE, A_BOLD));
    mvwprintw(win, 0, 0,
              " UART TERMINAL | F1/Esc: debugger | Ctrl+R reset | "
              "Ctrl+X quit | RX: %u | STATUS: %02X ",
              (unsigned int)uart_get_rx_count(), status);
    wattroff(win, color_attr(PAIR_TITLE, A_BOLD));

    if (content_h <= 0 || content_w <= 0) {
        wnoutrefresh(win);
        doupdate();
        return;
    }

    if (len > (size_t)(content_h * content_w)) {
        start = len - (size_t)(content_h * content_w);
    }

    for (size_t i = start; i < len; i++) {
        char ch = buffer[i];

        if (ch == '\n') {
            y++;
            x = 0;
        } else {
            mvwaddch(win, y, x, ch);
            x++;
            if (x >= content_w) {
                y++;
                x = 0;
            }
        }

        if (y >= rows) break;
    }

    wnoutrefresh(win);
    doupdate();
}

static void display_footer_window(void) {
    WINDOW* win = layout.footer;

    if (win == NULL) return;

    draw_box(win, "Controls", 0);
    mvwprintw(win, 1, 2,
              "Enter step | Space run/pause | n step 10 | Tab/Shift+Tab pane | "
              "Up/Down/Pg/Home/End scroll | F2 UART terminal | "
              "c clear terminal | r reset | q quit");
    wnoutrefresh(win);
}

void interface_display_header(void) {
    ensure_colors();
    if (!layout_ready()) {
        display_too_small();
        return;
    }
    display_header_window();
}

void interface_display_cpu(void) {
    if (!layout_ready()) return;

    display_register_window();
    display_flags_window();
    display_fetch_window();
    display_vectors_window();
    display_trace_window();
    display_performance_window();
}

void interface_display_mem(void) {
    if (!layout_ready()) return;

    for (enum memory_view view = MEM_VIEW_ZERO_PAGE; view < MEM_VIEW_COUNT;
         view++) {
        display_memory_pane(view);
    }
    display_terminal_window();
    display_footer_window();
    doupdate();
}

void interface_handle_resize(void) {
    endwin();
    refresh();
    clear();
    delete_layout();
    create_layout();
}

void interface_shutdown(void) {
    delete_layout();
}

void interface_next_mem_view(void) {
    ui_state.active_view = (ui_state.active_view + 1) % MEM_VIEW_COUNT;
}

void interface_prev_mem_view(void) {
    ui_state.active_view =
        (ui_state.active_view + MEM_VIEW_COUNT - 1) % MEM_VIEW_COUNT;
}

void interface_scroll_mem_view(int rows) {
    int next_scroll;
    int screen_rows, screen_cols;
    int step;

    screen_size(&screen_rows, &screen_cols);
    step = bytes_per_row_for_width(pane_width(screen_cols));
    next_scroll = ui_state.scroll[ui_state.active_view] +
                  (rows * step);

    if (next_scroll < 0) {
        ui_state.scroll[ui_state.active_view] = 0;
    } else if ((uint16_t)next_scroll > max_scroll_for_view(ui_state.active_view)) {
        ui_state.scroll[ui_state.active_view] =
            max_scroll_for_view(ui_state.active_view);
    } else {
        ui_state.scroll[ui_state.active_view] = (uint16_t)next_scroll;
    }
}

void interface_mem_view_home(void) {
    ui_state.scroll[ui_state.active_view] = 0;
}

void interface_mem_view_end(void) {
    ui_state.scroll[ui_state.active_view] = max_scroll_for_view(ui_state.active_view);
}

enum ui_mode interface_get_mode(void) {
    return ui_state.mode;
}

void interface_set_mode(enum ui_mode mode) {
    ui_state.mode = mode;
}
