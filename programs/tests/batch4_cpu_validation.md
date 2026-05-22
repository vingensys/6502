# Batch 4 CPU Validation Tests

`cpu_batch4_runner.c` validates ALU and flag behavior without starting ncurses.
It compiles directly against the emulator CPU and memory modules, loads tiny
programs at `$8000`, executes fixed instruction counts, and asserts registers,
memory, and flags.

Run:

```sh
programs/tests/run_cpu_batch4.sh
```

Covered behavior:

| Area | Coverage |
|---|---|
| Logical ops | AND, ORA, EOR result, Z/N flags, and C/V preservation across immediate, zero page, absolute, indexed, and indirect representative modes. |
| Comparisons | CMP, CPX, CPY equal, greater-than, and less-than cases; Z, C, and N behavior. |
| ADC binary | No carry, incoming carry, carry out, zero result, negative result, overflow behavior, and decimal-clear path. |
| SBC binary | No borrow, incoming borrow, borrow result, zero result, negative result, overflow behavior, and decimal-clear path. |
| Shifts | ASL and LSR accumulator and memory forms; result, C, Z, and N behavior. |
| Rotates | ROL and ROR accumulator and memory forms; incoming C, outgoing C, result, Z, and N behavior. |
| INC/DEC | Zero and negative memory results for DEC and INC, including wraparound. |

Decimal-mode BCD arithmetic is intentionally not validated here because it is
not implemented yet. ADC/SBC remain partial for decimal mode.
