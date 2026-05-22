# Sample Cartridges

This directory is for small cartridge/user-program ROM images loaded at
`$8000-$BFFF`.

Generate the sample cartridge:

```sh
programs/carts/build_carts.sh
```

Generated cartridges:

| File | Behavior |
| --- | --- |
| `ok_once.bin` | Writes `OK` to UART `$D010`, then executes a NOP. |
| `ok_loop.bin` | Writes `OK\n` to UART `$D010`, then loops. |
| `uart_hi.bin` | Writes `HI` to UART `$D010`, then executes a NOP. |
| `uart_echo.bin` | Polls UART status, reads UART input, echoes it back, and loops. |
| `store_2a.bin` | Stores `$2A` at zero-page address `$0005`, then executes two NOPs. |
| `breakpoint_test.bin` | Writes `A`, executes a NOP at `$8005`, then writes `B` and loops. |

They can be run directly or from the monitor:

```sh
./bin/emulator.out --cart programs/carts/ok_once.bin
./bin/emulator.out --monitor --cart programs/carts/ok_once.bin
```
