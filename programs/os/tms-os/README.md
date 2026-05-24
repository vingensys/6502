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
| `RESET` | Re-enter the OS prompt. |

`PEEK`, `POKE`, `MEM`, and `RUN` are planned next.

## Build

```sh
cd programs/os/tms-os
make
```

Clean generated output:

```sh
make clean
```

## Planned Service Entry Points

TMS-OS exposes these initial service routines:

- `OS_PUTC`
- `OS_GETC`
- `OS_PUTS`
- `OS_NEWLINE`
- `OS_WARM_START`

The final fixed ABI addresses are not promised yet. They should be pinned and
documented before user cartridges depend on them.

## Status

Stage 1 bootable skeleton implemented:

- reset/startup assembly
- vectors at `$FFFA-$FFFF`
- UART console routines
- C kernel and shell loop
- `HELP`, `INFO`, and `RESET`
