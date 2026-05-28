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
| `echo_upper.bin` | Interactive UART cartridge that prints a prompt and echoes lowercase letters as uppercase. |
| `os_call_demo.bin` | Assembly cartridge that calls the fixed TMS-OS service ABI. |
| `c-sdk/build/os_call_demo_c.bin` | C cartridge built with `programs/carts/c-sdk`; calls the fixed TMS-OS service ABI. |
| `c-sdk/build/tms_calc.bin` | Interactive C calculator cartridge using TMS-OS service calls. |

They can be run directly or from the monitor:

```sh
./bin/emulator.out --cart programs/carts/ok_once.bin
./bin/emulator.out --monitor --cart programs/carts/ok_once.bin
```

The C SDK demo is meant to run under TMS-OS:

```sh
./bin/emulator.out --rom programs/os/tms-os/build/tmsos.bin --cart programs/carts/c-sdk/build/os_call_demo_c.bin --ui headless
```

Then at the TMS-OS prompt:

```text
RUN 8000
```

The calculator cartridge is also launched from TMS-OS:

```sh
./bin/emulator.out --rom programs/os/tms-os/build/tmsos.bin --cart programs/carts/c-sdk/build/tms_calc.bin --ui headless
```
