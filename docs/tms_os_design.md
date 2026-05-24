# TMS-OS Design

TMS-OS is a standalone ROM project for the emulator. It does not replace or
modify the existing Python-generated monitor in `programs/monitor/`. The
monitor remains available through `--monitor`; TMS-OS is booted explicitly as a
kernel ROM image.

## Goals

TMS-OS v0.1 provides a tiny ROM-resident operating environment for the current
official NMOS 6502 machine profile:

- ROM image loaded at `$E000-$FFFF`
- reset vector at `$FFFC/$FFFD` points to `$E000`
- IRQ/BRK and NMI vectors are explicitly owned by the OS image
- stack is initialized during startup
- UART at `$D010/$D011` is the console device
- a small command shell starts with `HELP`, `INFO`, and `RESET`

The first boot target is:

```text
TMS-OS 0.1
READY
>
```

## Source Layout

TMS-OS lives under a new standalone folder:

```text
programs/os/tms-os/
├── README.md
├── Makefile
├── linker.cfg
├── include/
│   └── tmsos.h
├── src/
│   ├── crt0.s
│   ├── vectors.s
│   ├── uart.s
│   ├── kernel.c
│   ├── shell.c
│   ├── memory.c
│   └── hex.c
└── build/
    └── tmsos.bin
```

The `build/` directory contains generated output. Source files, linker config, headers, and README are the maintainable inputs.

## Toolchain

TMS-OS should use a cc65-style toolchain:

- `ca65` for assembly startup, vectors, fixed entry points, and UART primitives
- `cc65`/`cl65` for C shell and kernel logic
- `ld65` with `linker.cfg` to place the ROM image at `$E000-$FFFF`

Python byte-array generation should not be used for TMS-OS, except for optional helper scripts that do not replace the assembler/linker flow.

## Memory Map

TMS-OS targets the emulator's current machine map:

| Address range | Purpose |
| --- | --- |
| `$0000-$00FF` | Zero page variables and C runtime scratch area |
| `$0100-$01FF` | 6502 hardware stack |
| `$0200-$7FFF` | RAM |
| `$8000-$BFFF` | Cartridge/user program ROM |
| `$C000-$CFFF` | Expansion RAM |
| `$D000-$DFFF` | I/O |
| `$D010` | UART data |
| `$D011` | UART status |
| `$E000-$FFFF` | TMS-OS kernel ROM |
| `$FFFA-$FFFF` | NMI, RESET, IRQ/BRK vectors |

The linker config must keep vectors at the end of the ROM image and place startup at `$E000`.

## Booting

TMS-OS should be built to:

```text
programs/os/tms-os/build/tmsos.bin
```

It should boot explicitly with:

```sh
./bin/emulator.out --rom programs/os/tms-os/build/tmsos.bin --ui headless
```

With a cartridge loaded at `$8000`:

```sh
./bin/emulator.out --rom programs/os/tms-os/build/tmsos.bin --cart programs/carts/echo_upper.bin --ui headless
```

The existing monitor shortcut remains unchanged:

```sh
./bin/emulator.out --monitor
```

## Initial Commands

TMS-OS v0.1 stage 1 shell commands:

| Command | Behavior |
| --- | --- |
| `HELP` | Print command summary. |
| `INFO` | Print OS, CPU, ROM, RAM, cartridge, and UART map summary. |
| `RESET` | Re-enter the OS warm start path. |

Planned follow-up commands:

| Command | Behavior |
| --- | --- |
| `PEEK hhhh` | Read and print one byte from address `hhhh`. |
| `POKE hhhh bb` | Write byte `bb` to address `hhhh`. |
| `MEM hhhh` | Dump memory starting at `hhhh`, initially one 16-byte line. |
| `RUN hhhh` | Jump to address `hhhh`, commonly `$8000` for a cartridge. |

Command parsing can be simple and uppercase-first. Lowercase support is useful, but not required for v0.1 if ROM space or implementation complexity becomes a concern.

## Stable Entry Points

TMS-OS should expose stable ROM entry points for programs and future tools:

| Entry point | Purpose |
| --- | --- |
| `OS_PUTC` | Write character in `A` to UART. |
| `OS_GETC` | Wait for and return a UART character in `A`. |
| `OS_PUTS` | Print a null-terminated string through UART. |
| `OS_NEWLINE` | Print newline sequence. |
| `OS_WARM_START` | Return to the OS shell prompt without a hardware reset. |

Stage 1 exposes these symbols internally, but the fixed user-program ABI
addresses are not promised yet. The exact addresses should either be fixed by
`vectors.s`/linker symbols or documented before programs begin depending on
them.

## File Responsibilities

| File | Responsibility |
| --- | --- |
| `crt0.s` | Reset entry at `$E000`, stack initialization, C runtime handoff. |
| `vectors.s` | NMI, RESET, IRQ/BRK vectors and fixed OS entry labels. |
| `uart.s` | Low-level UART polling, getc, putc, puts/newline entry points. |
| `kernel.c` | Boot banner, warm start, shell dispatch setup. |
| `shell.c` | Command parsing and command handlers. |
| `memory.c` | PEEK, POKE, MEM, and address-safe helpers. |
| `hex.c` | ASCII hex parsing and formatting. |
| `tmsos.h` | Shared constants, prototypes, and service declarations. |
| `linker.cfg` | ROM, vector, code, rodata, data, bss placement. |

## Implementation Order

1. Done: create the folder skeleton and cc65 build files.
2. Done: add minimal assembly startup and vectors so `$E000` boots.
3. Done: add UART primitives and a C-callable console layer.
4. Done: add `kernel.c` banner and warm-start prompt.
5. Done: add command parser and `HELP`/`INFO`/`RESET`.
6. Next: add `PEEK`, `POKE`, `MEM`, and `RUN`.
7. Next: add automated smoke tests using `--ui headless`.

## Non-Goals for v0.1

- no filesystem
- no multitasking
- no dynamic loader
- no assembler or disassembler shell commands
- no 65C02, NES/RP2A03, or undocumented opcode dependency
- no changes to `programs/monitor/`
- no changes to `--monitor`

## Compatibility Notes

TMS-OS v0.1 should target official NMOS 6502 only. Future 65C02 or NES-specific work should require explicit CPU profile selection and should not silently change TMS-OS behavior.
