# Echo Upper Cartridge Validation

`echo_upper_runner.c` loads `programs/carts/echo_upper.bin` directly at
`$8000`, queues `hello 6502` into the UART RX FIFO, and verifies the cartridge
prints `HELLO 6502`.

Run it with:

```sh
programs/tests/run_echo_upper.sh
```
