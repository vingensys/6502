# TMS-OS C Cartridge SDK

This is a minimal cc65-based SDK for building cartridge programs that run at
`$8000` and call TMS-OS through the fixed service ABI at `$E100-$E10C`.

It is intentionally small. It is not a full libc environment and does not
provide `printf`, `malloc`, files, `argc`/`argv`, or an operating-system
process model.

## Build

Install cc65 in Termux:

```sh
pkg install cc65
```

Build the demo cartridge:

```sh
cd programs/carts/c-sdk
make clean && make
```

The build emits:

```text
build/os_call_demo_c.bin
build/os_call_demo_c.map
```

## Run

From the repo root:

```sh
./bin/emulator.out --rom programs/os/tms-os/build/tmsos.bin --cart programs/carts/c-sdk/build/os_call_demo_c.bin --ui headless
```

At the TMS-OS prompt:

```text
RUN 8000
```

Expected output:

```text
HELLO FROM C CART
TMS-OS 0.1
READY
>
```

## C API

```c
void os_putc(char c);
char os_getc(void);
void os_puts(const char *s);
void os_newline(void);
void os_warm_start(void);
```

Example:

```c
#include "tmsos_cart.h"

int main(void) {
    os_puts("HELLO FROM C CART");
    os_newline();
    os_warm_start();
    return 0;
}
```

## Memory Contract

The cartridge SDK uses only user-owned memory:

| Address range | Use |
| --- | --- |
| `$0080-$0081` | TMS-OS `OS_PUTS` string pointer |
| `$0082-$00FF` | cc65 cartridge zero-page runtime workspace |
| `$0400-$7FFF` | C data, BSS, and software stack |
| `$8000-$BFFF` | cartridge ROM image |

The linker map should show `ZEROPAGE` starting at `$0082`, `DATA/BSS` in
`$0400-$7FFF`, and `STARTUP/CODE/RODATA` starting at `$8000`. `$80/$81` are
kept free for the OS string pointer ABI.
