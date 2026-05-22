# 6502 Emulator

A minimal, single-stepped and beginner friendly 6502 emulator written in C using ncurses for graphics.

![thumbnail](./images/thumbnail.png)

## DISCLAIMER

This main goal of this project is to understand how CPUs works by directly emulating one and to debug it by single stepping instructions. The code is meant to be readable and understandable, a lot of things could be done better, especially the graphics.

The emulator only shows you what's going on under the hood of a 6502 CPU, without displaying stuff graphically (you will only see hex digits). The example program simply caluclates `10*3` and it's not optimized.

## Run

You must have `ncurses` installed on your machine. This project was developed in a Linux environment.

```
make
```

```
./bin/emulator.out
```

Or you can run the _shortcut_ script

```
bash run.sh
```

### Boot configuration

By default, the emulator preserves the original simple behavior: it loads
`example.bin` as a cartridge at `$8000` and sets RESET to `$8000`.

```
./bin/emulator.out
```

Run a cartridge directly:

```
./bin/emulator.out --cart programs/carts/ok_once.bin
```

Boot the monitor ROM only:

```
./bin/emulator.out --monitor
```

Boot the monitor ROM with a cartridge at `$8000`:

```
./bin/emulator.out --monitor --cart programs/carts/ok_once.bin
```

Use the full explicit form:

```
./bin/emulator.out --cpu nmos6502 --rom programs/monitor/monitor.bin --cart programs/carts/ok_once.bin
```

The legacy form `./bin/emulator.out your_binary_here` is still accepted as a
cartridge shortcut. The legacy form `./bin/emulator.out --monitor myprog.bin`
is accepted as a deprecated alias for `--monitor --cart myprog.bin`.

## Code style

The paradigm I've chosen is `modular programming`, especially because this is C. System components aren't defined in a OOP way.

Everything is very verbose with a lot of comments.

## Design

The project is divided in multiple components:

-   **cpu**: here you will find the CPU itself, including main methods to interact with the memory
    -   **instructions handler**: here we handle OP codes
-   **mem**: 64KB machine address space with RAM, ROM, reserved I/O regions, and vectors
-   **peripherals**
    -   **interface**: everyhting ncurses related
    -   **keyboard handler**: listener for key presses

CPU opcode coverage is tracked in `docs/opcode_coverage.md`. Future CPU
variant/profile planning is tracked in `docs/cpu_profiles.md`.

## Memory Map

The emulator exposes a 64KB 6502 address space. CPU instruction reads and writes go through the memory bus helpers in `mem.c`.

| Address range | Region |
| --- | --- |
| `$0000-$00FF` | Zero Page |
| `$0100-$01FF` | Stack |
| `$0200-$7FFF` | RAM |
| `$8000-$BFFF` | User Program ROM / cartridge / loaded program |
| `$C000-$CFFF` | Expansion RAM or banked region reserved for later |
| `$D000-$D00F` | VIA 6522 #1 reserved |
| `$D010-$D01F` | UART / ACIA 6551 reserved |
| `$D020-$D02F` | Timer / system control reserved |
| `$D030-$DFFF` | I/O expansion reserved |
| `$E000-$FFFF` | Kernel ROM / monitor ROM |

The vector table lives at the end of kernel ROM:

| Address range | Vector |
| --- | --- |
| `$FFFA-$FFFB` | NMI |
| `$FFFC-$FFFD` | RESET |
| `$FFFE-$FFFF` | IRQ/BRK |

By default, cartridge binaries are loaded at `$8000` and the RESET vector is set
to `$8000`. If a kernel ROM is loaded at `$E000`, RESET is set to `$E000`.

## UI modes

The emulator has two ncurses screens:

-   **DEBUGGER mode**: shows CPU registers, flags, fetch/decode, trace, vectors, memory panes, and the small UART output pane. Debugger controls are active here: Enter steps, Space runs or pauses, `n` runs 10 instructions, `q` quits, `r` resets, and the memory pane navigation keys scroll/select panes.
-   **UART TERMINAL mode**: a full-screen terminal connected to the emulated UART. Press F2 from DEBUGGER mode to enter it. Press F1 or Esc to return to the debugger. In this mode, printable keys are sent to the UART RX FIFO, Enter sends newline, and Backspace sends `0x08`. A typed `q` is sent to the emulated UART instead of quitting the emulator.

Entering UART TERMINAL mode starts the run loop so programs that poll the UART can respond immediately.

### UART terminal test

The first memory-mapped peripheral is a simple UART:

| Address | Register |
| --- | --- |
| `$D010` | UART data |
| `$D011` | UART status, bit 0 = TX ready, bit 1 = RX ready |

Writes to `$D010` append ASCII text to the debugger's Terminal pane. Reads from `$D010` consume queued keyboard input when bit 1 is set in `$D011`.

To create a tiny program that prints `HI`:

```
programs/carts/build_carts.sh
./bin/emulator.out --cart programs/carts/uart_hi.bin
```

Step through the program or run it; the Terminal pane should show `HI`.

To create a tiny echo loop:

```
programs/carts/build_carts.sh
./bin/emulator.out --cart programs/carts/uart_echo.bin
```

Press F2 to enter UART TERMINAL mode, then type printable keys. The program polls `$D011`, reads from `$D010`, and writes the byte back to `$D010`, so typed characters appear in the terminal screen. Press F1 or Esc to return to the debugger.

## Monitor ROM

The emulator includes a tiny WOZMON-inspired monitor ROM that boots from kernel ROM at `$E000` and talks through the UART.

Build the ROM:

```
programs/monitor/build_monitor.sh
```

Run monitor mode:

```
./bin/emulator.out --monitor
```

Monitor mode is a shortcut for loading `programs/monitor/monitor.bin` at
`$E000` and setting the RESET vector `$FFFC/$FFFD` to `$E000`. It does not
automatically load a cartridge. You can load a `$8000` cartridge while still
booting into the monitor:

```
./bin/emulator.out --monitor --cart myprog.bin
```

Press F2 to switch to UART TERMINAL mode and start interacting with the monitor. The expected startup text is:

```
TMS6502 MONITOR
>
```

Initial commands are compact and use hexadecimal:

```
?
M0000
W00052A
M0000
R
G8000
```

`?` prints help. `Mhhhh` dumps 16 bytes. `Whhhhbb` writes one byte. `Ghhhh` jumps to an address. `R` re-enters the monitor prompt without resetting the emulator. `Rhhhh` remains accepted as a legacy alias for `Ghhhh`.

For example, after `./bin/emulator.out --monitor --cart myprog.bin`, use
`G8000` at the monitor prompt to jump from the monitor into `myprog.bin`.

You can also load another kernel ROM explicitly:

```
./bin/emulator.out --rom programs/monitor/monitor.bin
```

## Dump feature

After quitting, the program dumps its memory to a `.bin` file.

## Example program

The loaded program multiplies 10 by 3, in order to try it you must single step instructions until you see `1E` (30) in the third memory cell in the zero page. You can continue to single step it but nothing will happen.

## TODO

Do you want to contribute? Here are some things that are still a WIP.

-   [x] check for errors on cpu_fetch() calls
    -   due to uint16_t always being between `0x0000` and `0xFFFF` we don't have to perform extra checking while fetching memory.
-   [ ] add remaining comments to `instructions.c`
-   [ ] create a better interface
-   [x] add the possibility to load custom programs

## References

-   [obelisk.me.uk/6502](http://www.obelisk.me.uk/6502/)

## Credits

Original project by Leonardo Folgoni:
https://github.com/f0lg0/6502

This project preserves the original MIT license and attribution.

Debugger UI improvements, 64KB memory bus refactor, reset vector implementation, ncurses debugger panes, tracing system, and UART terminal additions were developed with assistance from OpenAI Codex under direction of Vinod S Nair.
