#define _POSIX_C_SOURCE 200809L

#include <ncurses.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "cpu/cpu.h"
#include "mem/mem.h"
#include "peripherals/interface.h"
#include "peripherals/kinput.h"
#include "peripherals/uart.h"

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
    enum ui_mode start_ui_mode;
};

uint8_t DEBUG = 0;
static volatile sig_atomic_t headless_should_quit = 0;

static void handle_sigint(int signum) {
    (void)signum;
    headless_should_quit = 1;
}

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
            "[--monitor] [--ui debugger|terminal|headless]\n"
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

static uint8_t parse_ui_mode(const char* name, enum ui_mode* mode) {
    if (strcmp(name, "debugger") == 0) {
        *mode = DEBUGGER_MODE;
        return 0;
    }
    if (strcmp(name, "terminal") == 0) {
        *mode = UART_TERMINAL_MODE;
        return 0;
    }
    if (strcmp(name, "headless") == 0) {
        *mode = HEADLESS_MODE;
        return 0;
    }

    fprintf(stderr,
            "[FAILED] Unknown UI mode '%s'. Supported: debugger, terminal.\n",
            name);
    return 1;
}

static uint8_t parse_args(int argc, char** argv, struct emulator_config* config) {
    int i;

    config->cpu_profile = CPU_PROFILE_NMOS6502;
    config->rom_path = NULL;
    config->cart_path = NULL;
    config->monitor_shortcut_enabled = 0;
    config->start_ui_mode = DEBUGGER_MODE;

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
        } else if (strcmp(argv[i], "--ui") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "[FAILED] --ui requires a mode.\n");
                return 1;
            }
            i++;
            if (parse_ui_mode(argv[i], &config->start_ui_mode) != 0) {
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

static void headless_restore_terminal(const struct termios* original_termios,
                                      int restore_termios,
                                      int original_stdin_flags) {
    if (restore_termios) {
        tcsetattr(STDIN_FILENO, TCSANOW, original_termios);
    }
    if (original_stdin_flags >= 0) {
        fcntl(STDIN_FILENO, F_SETFL, original_stdin_flags);
    }
}

static int run_headless(void) {
    struct termios original_termios;
    struct termios raw_termios;
    const struct timespec loop_sleep = {0, 1000000};
    int restore_termios = 0;
    int original_stdin_flags;
    size_t tx_pos = 0;

    signal(SIGINT, handle_sigint);
    setvbuf(stdout, NULL, _IONBF, 0);

    original_stdin_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (original_stdin_flags >= 0) {
        fcntl(STDIN_FILENO, F_SETFL, original_stdin_flags | O_NONBLOCK);
    }

    if (isatty(STDIN_FILENO) &&
        tcgetattr(STDIN_FILENO, &original_termios) == 0) {
        raw_termios = original_termios;
        raw_termios.c_iflag &= (tcflag_t)~(BRKINT | ICRNL | INPCK | ISTRIP |
                                           IXON);
        /* Keep output processing enabled so '\n' still returns the cursor to
         * column 0 on an interactive terminal. Only input needs raw-ish mode. */
        raw_termios.c_cflag |= (CS8);
        raw_termios.c_lflag &= (tcflag_t)~(ECHO | ICANON | IEXTEN | ISIG);
        raw_termios.c_cc[VMIN] = 0;
        raw_termios.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &raw_termios);
        restore_termios = 1;
    }

    while (!headless_should_quit) {
        uint8_t input_buf[128];
        ssize_t bytes_read;
        const char* tx;
        size_t tx_len;

        do {
            bytes_read = read(STDIN_FILENO, input_buf, sizeof(input_buf));
            if (bytes_read > 0) {
                for (ssize_t i = 0; i < bytes_read; i++) {
                    if (input_buf[i] == 0x03 || input_buf[i] == 0x18) {
                        headless_should_quit = 1;
                    } else if (input_buf[i] == 0x12) {
                        cpu_reset();
                    } else {
                        uart_enqueue_input(input_buf[i]);
                    }
                }
            }
        } while (bytes_read > 0 && !headless_should_quit);

        for (uint16_t i = 0; i < 500; i++) {
            cpu_exec();
        }

        tx = uart_get_buffer();
        tx_len = uart_get_buffer_len();
        if (tx_pos > tx_len) tx_pos = 0;
        if (tx_len > tx_pos) {
            fwrite(tx + tx_pos, 1, tx_len - tx_pos, stdout);
            tx_pos = tx_len;
        }

        if (bytes_read < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            headless_should_quit = 1;
        }
        nanosleep(&loop_sleep, NULL);
    }

    headless_restore_terminal(&original_termios, restore_termios,
                              original_stdin_flags);
    return 0;
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

    if (config.start_ui_mode == HEADLESS_MODE) {
        return run_headless();
    }

    WINDOW* win;
    if ((win = initscr()) == NULL) {
        fprintf(stderr, "[FAILED] Error initialising ncurses.\n");
        exit(1);
    }
    curs_set(0);
    raw();
    noecho();
    keypad(stdscr, TRUE);

    timeout(0);
    interface_set_mode(config.start_ui_mode);
    if (config.start_ui_mode == UART_TERMINAL_MODE) {
        kinput_set_running(1);
    }
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
