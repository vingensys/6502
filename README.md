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

### Loading a custom program

By default, the emulator loads `example.bin`. If you want to load a custom binary file, just provide the path as the second argument while launching the emulator:

```
./bin/emulator.out your_binary_here
```

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

By default, binaries are loaded at `$8000` and the RESET vector is set to `$8000`.

### UART terminal test

The first memory-mapped peripheral is a simple output-only UART:

| Address | Register |
| --- | --- |
| `$D010` | UART data |
| `$D011` | UART status, bit 0 = TX ready |

Writes to `$D010` append ASCII text to the debugger's Terminal pane. Input is not implemented yet. To create a tiny program that prints `HI`:

```
printf '\xA9\x48\x8D\x10\xD0\xA9\x49\x8D\x10\xD0\xEA' > uart_hi.bin
./bin/emulator.out uart_hi.bin
```

Step through the program or run it; the Terminal pane should show `HI`.

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
