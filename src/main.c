#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu/cpu.h"
#include "mem/mem.h"
#include "peripherals/interface.h"
#include "peripherals/kinput.h"

#define DEFAULT_MONITOR_ROM "programs/monitor/monitor.bin"
#define DEFAULT_CART_ROM "example.bin"

enum cpu_profile {
    CPU_PROFILE_NMOS6502,
};

struct emulator_config {
    enum cpu_profile cpu_profile;
    const char* rom_path;
    const char* cart_path;
    uint8_t monitor_shortcut_enabled;
};

uint8_t DEBUG = 0;

static void draw_interface(void) {
    if (interface_get_mode() == UART_TERMINAL_MODE) {
        interface_display_uart_terminal();
        return;
    }

    interface_display_header();
    interface_display_cpu();
    interface_display_mem();
}

static void print_usage(const char* program) {
    fprintf(stderr,
            "Usage: %s [--cpu nmos6502] [--rom path] [--cart path] "
            "[--monitor]\n"
            "       %s [legacy-cart.bin]\n"
            "       %s --monitor [legacy-cart.bin]\n",
            program, program, program);
}

static uint8_t parse_cpu_profile(const char* name, enum cpu_profile* profile) {
    if (strcmp(name, "nmos6502") == 0) {
        *profile = CPU_PROFILE_NMOS6502;
        return 0;
    }

    fprintf(stderr, "[FAILED] Unknown CPU profile '%s'. Supported: nmos6502.\n",
            name);
    return 1;
}

static uint8_t parse_args(int argc, char** argv, struct emulator_config* config) {
    int i;

    config->cpu_profile = CPU_PROFILE_NMOS6502;
    config->rom_path = NULL;
    config->cart_path = NULL;
    config->monitor_shortcut_enabled = 0;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--cpu") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "[FAILED] --cpu requires a profile name.\n");
                return 1;
            }
            i++;
            if (parse_cpu_profile(argv[i], &config->cpu_profile) != 0) {
                return 1;
            }
        } else if (strcmp(argv[i], "--cart") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "[FAILED] --cart requires a cartridge path.\n");
                return 1;
            }
            config->cart_path = argv[++i];
        } else if (strcmp(argv[i], "--rom") == 0 ||
                   strcmp(argv[i], "--kernel") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "[FAILED] %s requires a ROM path.\n", argv[i]);
                return 1;
            }
            if (strcmp(argv[i], "--kernel") == 0) {
                fprintf(stderr,
                        "[WARN] --kernel is deprecated; use --rom instead.\n");
            }
            config->rom_path = argv[++i];
        } else if (strcmp(argv[i], "--monitor") == 0) {
            config->monitor_shortcut_enabled = 1;
            config->rom_path = DEFAULT_MONITOR_ROM;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "[FAILED] Unknown option '%s'.\n", argv[i]);
            return 1;
        } else {
            if (config->cart_path != NULL) {
                fprintf(stderr,
                        "[FAILED] Multiple cartridge images provided. Use one "
                        "--cart path.\n");
                return 1;
            }
            if (config->monitor_shortcut_enabled) {
                fprintf(stderr,
                        "[WARN] '--monitor %s' is deprecated; use "
                        "'--monitor --cart %s'.\n",
                        argv[i], argv[i]);
            }
            config->cart_path = argv[i];
        }
    }

    if (config->rom_path == NULL && config->cart_path == NULL) {
        config->cart_path = DEFAULT_CART_ROM;
    }

    return 0;
}

static void configure_cpu_profile(enum cpu_profile profile) {
    switch (profile) {
        case CPU_PROFILE_NMOS6502:
            /* The emulator currently implements the official NMOS 6502
             * profile. Future profiles should attach CPU-specific dispatch
             * and behavior here. */
            break;
    }
}

int main(int argc, char** argv) {
    struct emulator_config config;

    if (parse_args(argc, argv, &config) != 0) {
        print_usage(argv[0]);
        exit(1);
    }

    configure_cpu_profile(config.cpu_profile);
    mem_init_machine(config.rom_path, config.cart_path);

    cpu_init();
    cpu_reset();

    WINDOW* win;
    if ((win = initscr()) == NULL) {
        fprintf(stderr, "[FAILED] Error initialising ncurses.\n");
        exit(1);
    }
    curs_set(0);
    noecho();
    keypad(stdscr, TRUE);

    timeout(0);
    draw_interface();

    do {
        timeout(kinput_is_running() ? 0 : -1);
        draw_interface();

        kinput_listen();
        if (kinput_is_running()) {
            uint16_t instruction_budget =
                interface_get_mode() == UART_TERMINAL_MODE ? 200 : 1;
            for (uint16_t i = 0; i < instruction_budget; i++) {
                cpu_exec();
            }
            napms(interface_get_mode() == UART_TERMINAL_MODE ? 5 : 30);
        }
    } while (!kinput_should_quit());

    (void)win;
    interface_shutdown();
    endwin();

    mem_dump();
    return 0;
}
