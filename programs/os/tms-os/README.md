# TMS-OS

TMS-OS is a standalone ROM operating environment for the emulator. It is
separate from the existing monitor in `programs/monitor/` and is built with a
cc65-style assembler/linker flow, not Python byte-array generation.

## Target

TMS-OS v0.1 builds a kernel ROM image at:

```text
programs/os/tms-os/build/tmsos.bin
```

The ROM is loaded at `$E000-$FFFF`, with the RESET vector pointing to `$E000`.

Boot it explicitly with:

```sh
./bin/emulator.out --rom programs/os/tms-os/build/tmsos.bin --ui headless
```

Boot with a cartridge at `$8000`:

```sh
./bin/emulator.out --rom programs/os/tms-os/build/tmsos.bin --cart programs/carts/echo_upper.bin --ui headless
```

Boot with a cartridge that calls TMS-OS services:

```sh
programs/carts/build_carts.sh
./bin/emulator.out --rom programs/os/tms-os/build/tmsos.bin --cart programs/carts/os_call_demo.bin --ui headless
```

Boot with the minimal C SDK cartridge:

```sh
cd programs/carts/c-sdk
make clean && make
cd ../../..
./bin/emulator.out --rom programs/os/tms-os/build/tmsos.bin --cart programs/carts/c-sdk/build/os_call_demo_c.bin --ui headless
```

The existing monitor remains unchanged and is still launched with:

```sh
./bin/emulator.out --monitor
```

## Planned Layout

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

## Toolchain Direction

Use cc65 tooling. On Termux:

```sh
pkg install cc65
```

The build uses:

- `ca65` for startup, vectors, UART primitives, and fixed entry points
- `cc65`/`cl65` for C shell and kernel logic
- `ld65` with `linker.cfg` to place code, data, and vectors in `$E000-$FFFF`

Each build also emits `build/tmsos.map` for auditing ROM, RAM, and zero-page
placement.

Python helper scripts may be used for optional checks, but not as the OS ROM generator.

## v0.1 Boot Text

```text
TMS-OS 0.1
READY
>
```

## v0.1 Commands

| Command | Behavior |
| --- | --- |
| `HELP` | Print command summary. |
| `INFO` | Print OS and memory map summary. |
| `MAP` | Print the TMS-OS memory ownership map. |
| `MEM hhhh` | Dump 16 bytes starting at address `hhhh`. |
| `MEM` | Continue dumping from the next address after the previous `MEM`. |
| `NEXT` | Continue dumping from the next address after the previous `MEM`. |
| `PEEK hhhh` | Print one byte from address `hhhh`. |
| `POKE hhhh bb` | Write byte `bb` to safe user RAM/ZP and print confirmation. |
| `RUN hhhh` | Jump to address `hhhh`, commonly `$8000` for a cartridge. |
| `RESET` | Re-enter the OS prompt. |

Short aliases are also accepted: `?` for `HELP`, `M hhhh` for `MEM hhhh`,
and `G hhhh` for `RUN hhhh`.

Example:

```text
MEM 8000
8000: A9 48 8D 10 D0 ...
PEEK 8000
8000 = A9
POKE 0400 2A
0400 = 2A OK
RUN 8000
```

`POKE` refuses OS-owned, I/O, cartridge ROM, and OS ROM addresses:

```text
POKE 0200 2A
PROTECTED
```

## Memory Ownership

TMS-OS v0.1 publishes this memory contract:

| Address range | Owner / use |
| --- | --- |
| `$0000-$007F` | TMS-OS and cc65 zero-page workspace |
| `$0080-$00FF` | User zero-page |
| `$0100-$01FF` | CPU hardware stack |
| `$0200-$02FF` | OS line input buffer, data, and BSS |
| `$0300-$03FF` | OS scratch RAM and cc65 software stack |
| `$0400-$7FFF` | User RAM |
| `$8000-$BFFF` | Cartridge/user program ROM |
| `$C000-$CFFF` | Expansion RAM |
| `$D000-$DFFF` | I/O |
| `$E000-$FFFF` | TMS-OS ROM |

The current linker map verifies zero-page use within `$0000-$001C`,
`DATA/BSS` within `$0200-$0222`, and the cc65 software stack initialized at
`$03FF`. User programs should treat `$0080-$00FF`, `$0400-$7FFF`, and
`$C000-$CFFF` as the writable regions exposed by this contract.

## TMS-OS Service ABI

Cartridge programs may call a small fixed jump table in OS ROM instead of
hardcoding UART registers. The jump table starts at `$E100`; each entry is a
3-byte `JMP` to the current implementation.

| Address | Entry point | Calling convention |
| ---: | --- | --- |
| `$E100` | `OS_WARM_START` | `JSR $E100`; returns to the TMS-OS warm-start prompt and does not return to caller. |
| `$E103` | `OS_PUTC` | `A` = byte to print; `JSR $E103`; preserves `X` and `Y`; `A` may be clobbered. |
| `$E106` | `OS_GETC` | `JSR $E106`; blocks until input; returns byte in `A`; preserves `X` and `Y`. |
| `$E109` | `OS_PUTS` | `$80/$81` = pointer to NUL-terminated string; `JSR $E109`; preserves `X` and `Y`; `A` may be clobbered. |
| `$E10C` | `OS_NEWLINE` | `JSR $E10C`; prints the OS newline convention; preserves `X` and `Y`; `A` may be clobbered. |

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

The sample cartridge `programs/carts/os_call_demo.bin` uses this ABI to print
`HELLO FROM OS ABI`, then returns to the TMS-OS prompt through
`OS_WARM_START`.

The minimal C cartridge SDK in `programs/carts/c-sdk/` wraps this ABI as:

```c
void os_putc(char c);
char os_getc(void);
void os_puts(const char *s);
void os_newline(void);
void os_warm_start(void);
```

The SDK linker config places cc65 zero-page runtime state at `$0082-$00FF`,
leaving `$80/$81` free for the `OS_PUTS` ABI pointer. C data, BSS, and the
software stack live in user RAM `$0400-$7FFF`; cartridge code starts at
`$8000`.

## Build

```sh
cd programs/os/tms-os
make
```

Clean generated output:

```sh
make clean
```

## Service Entry Points

The fixed service ABI above is the initial user-program interface. User
cartridges should call the `$E100-$E10C` jump table instead of depending on the
current implementation body addresses shown elsewhere in `build/tmsos.map`.

## Status

Stage 1 bootable skeleton implemented:

- reset/startup assembly
- vectors at `$FFFA-$FFFF`
- UART console routines
- fixed service ABI jump table at `$E100-$E10E`
- C kernel and shell loop
- `HELP`, `INFO`, `MAP`, `MEM`, `PEEK`, `POKE`, `RUN`, and `RESET`
