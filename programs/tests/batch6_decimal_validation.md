# Batch 6A Decimal ADC/SBC Validation Tests

`cpu_batch6_decimal_runner.c` validates NMOS 6502 decimal-mode ADC/SBC without
starting ncurses. It loads tiny binaries at `$8000`, sets Decimal mode with
`SED`, executes fixed instruction counts, and asserts accumulator results and
status flags.

Run:

```sh
programs/tests/run_cpu_batch6_decimal.sh
```

Covered behavior:

| Area | Coverage |
| --- | --- |
| ADC decimal arithmetic | `15 + 27 = 42`, `99 + 01 = 00` with Carry set, incoming Carry, zero result, and bit-7 result behavior. |
| ADC addressing | Immediate, zero page, absolute, absolute,X, and `(indirect),Y`. |
| SBC decimal arithmetic | `42 - 27 = 15`, `00 - 01 = 99` with Carry clear, no-borrow Carry set, borrow Carry clear, and incoming borrow behavior. |
| SBC addressing | Immediate, zero page, absolute, absolute,X, and `(indirect,X)`. |
| Flags | C/Z/N are checked from the decimal-adjusted result. V follows the binary overflow calculation from the unadjusted operation, which is the behavior this emulator documents for NMOS decimal mode. |

Binary-mode ADC/SBC remains covered by Batch 4.
