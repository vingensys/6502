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
- a small command shell starts with `HELP`, `INFO`, `MAP`, `MEM`, `PEEK`,
  `POKE`, `RUN`, and `RESET`

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
│   ├── abi.s
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

TMS-OS v0.1 further divides RAM into a published ownership contract:

| Address range | Owner / use |
| --- | --- |
| `$0000-$007F` | TMS-OS and cc65 zero-page workspace |
| `$0080-$00FF` | User zero-page |
| `$0100-$01FF` | CPU hardware stack |
| `$0200-$02FF` | OS line input buffer, data, and BSS |
| `$0300-$03FF` | OS scratch RAM and cc65 software stack |
| `$0400-$7FFF` | User RAM |
| `$C000-$CFFF` | Expansion RAM |

The linker config constrains `DATA` and `BSS` to `$0200-$03FF`; startup
initializes the cc65 software stack pointer to `$03FF`, growing down within OS
RAM. The current `build/tmsos.map` verifies zero-page use within
`$0000-$001C` and `DATA/BSS` within `$0200-$0222`.

Shell reads (`MEM` and `PEEK`) intentionally allow inspecting the full 64KB
machine address space. `POKE` refuses OS-owned, stack, cartridge ROM, I/O, and
OS ROM ranges by default. Safe writable ranges are user zero-page
`$0080-$00FF`, user RAM `$0400-$7FFF`, and expansion RAM `$C000-$CFFF`.

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

The `programs/carts/os_call_demo.bin` cartridge demonstrates the fixed TMS-OS
service ABI by printing through OS routines and returning to the OS prompt.
The `programs/carts/c-sdk/` project provides a minimal cc65 C cartridge SDK
that builds `build/os_call_demo_c.bin` against the same ABI.

The existing monitor shortcut remains unchanged:

```sh
./bin/emulator.out --monitor
```

## Initial Commands

TMS-OS v0.1 shell commands:

| Command | Behavior |
| --- | --- |
| `HELP` | Print command summary. |
| `INFO` | Print OS, CPU, ROM, RAM, cartridge, and UART map summary. |
| `MAP` | Print the full TMS-OS memory ownership map. |
| `MEM hhhh` | Dump 16 bytes starting at address `hhhh`. |
| `MEM` | Continue dumping from the next address after the previous `MEM`. |
| `NEXT` | Continue dumping from the next address after the previous `MEM`. |
| `PEEK hhhh` | Read and print one byte from address `hhhh`. |
| `POKE hhhh bb` | Write byte `bb` to safe user RAM/ZP and print confirmation. |
| `RUN hhhh` | Jump to address `hhhh`, commonly `$8000` for a cartridge. |
| `RESET` | Re-enter the OS warm start path. |

Short aliases are supported for common monitor-style use: `?` for `HELP`, `M`
for `MEM`, and `G` for `RUN`. Command parsing is intentionally simple and
case-insensitive for command names and hexadecimal digits. `POKE` prints
`PROTECTED` when the target address is outside the safe writable contract.

## Stable Entry Points

TMS-OS exposes stable ROM entry points for cartridges and future tools through
a fixed jump table at `$E100-$E10E`. Each entry is a 3-byte `JMP`, so the
implementation bodies may move without breaking cartridge binaries.

| Address | Entry point | Purpose |
| ---: | --- | --- |
| `$E100` | `OS_WARM_START` | Return to the OS shell prompt without a hardware reset; does not return to caller. |
| `$E103` | `OS_PUTC` | Write character in `A` to UART. |
| `$E106` | `OS_GETC` | Wait for and return a UART character in `A`. |
| `$E109` | `OS_PUTS` | Print a NUL-terminated string through UART using pointer `$80/$81`. |
| `$E10C` | `OS_NEWLINE` | Print newline sequence. |

`OS_PUTC`, `OS_GETC`, `OS_PUTS`, and `OS_NEWLINE` preserve `X` and `Y` where
practical. `A` may be clobbered except that `OS_GETC` returns the received byte
in `A`.

Example cartridge code:

```asm
        lda #<msg
        sta $80
        lda #>msg
        sta $81
        jsr $E109       ; OS_PUTS
        jsr $E10C       ; OS_NEWLINE
        jsr $E100       ; OS_WARM_START

msg:
        .byte "HELLO FROM CART", 0
```

The build force-imports the ABI labels so `build/tmsos.map` lists and verifies
`OS_WARM_START = $E100`, `OS_PUTC = $E103`, `OS_GETC = $E106`,
`OS_PUTS = $E109`, and `OS_NEWLINE = $E10C`.

## C Cartridge SDK

`programs/carts/c-sdk/` is a minimal cc65-based SDK for C cartridges. It is not
a full libc environment: no `printf`, `malloc`, file I/O, `argc`/`argv`, or
environment support is provided.

The SDK exposes friendly wrappers:

```c
void os_putc(char c);
char os_getc(void);
void os_puts(const char *s);
void os_newline(void);
void os_warm_start(void);
```

The cartridge linker config keeps the C runtime inside user-owned memory:

| Address range | SDK use |
| --- | --- |
| `$0080-$0081` | TMS-OS `OS_PUTS` string pointer |
| `$0082-$00FF` | cc65 zero-page runtime workspace |
| `$0400-$7FFF` | C data, BSS, and software stack |
| `$8000-$BFFF` | cartridge ROM image |

`build/os_call_demo_c.map` should show `ZEROPAGE` starting at `$0082`,
`DATA/BSS` in `$0400-$7FFF`, and `STARTUP/CODE/RODATA` starting at `$8000`.

## File Responsibilities

| File | Responsibility |
| --- | --- |
| `crt0.s` | Reset entry at `$E000`, stack initialization, C runtime handoff. |
| `abi.s` | Fixed `$E100` service jump table and cartridge ABI wrappers. |
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
6. Done: add `PEEK`, `POKE`, `MEM`, and `RUN`.
7. Done: define fixed `$E100` service ABI and add `os_call_demo` cartridge.
8. Done: add minimal cc65 C cartridge SDK and C ABI demo cartridge.
9. Next: add automated TMS-OS smoke tests using `--ui headless`.

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
