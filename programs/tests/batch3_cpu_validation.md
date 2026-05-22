# Batch 3 CPU Validation Tests

`cpu_batch3_runner.c` validates load/store addressing modes and memory-bus
behavior without starting ncurses. It compiles directly against the emulator CPU
and memory modules, loads tiny binaries at `$8000`, prepares RAM through the
bus, executes fixed instruction counts, and asserts registers, flags, memory,
and selected cycle counts.

Run:

```sh
programs/tests/run_cpu_batch3.sh
```

Covered behavior:

| Area | Coverage |
|---|---|
| LDA | Immediate, zero page, zero page,X, absolute, absolute,X, absolute,Y, `(indirect,X)`, `(indirect),Y`. |
| LDX | Immediate, zero page, zero page,Y, absolute, absolute,Y. |
| LDY | Immediate, zero page, zero page,X, absolute, absolute,X. |
| Load flags | Z is checked for zero loads; N is checked for bit-7 loads. |
| STA | Zero page, zero page,X, absolute, absolute,X, absolute,Y, `(indirect,X)`, `(indirect),Y`. |
| STX | Zero page, zero page,Y, absolute. |
| STY | Zero page, zero page,X, absolute. |
| Store flags | Stores are checked to preserve SR. |
| Zero-page wraparound | `$F0 + $20 -> $10` for indexed zero-page modes; `(indirect,X)` wraps pointer fetch; `(indirect),Y` wraps high-byte pointer fetch from `$FF` to `$00`. |
| Absolute indexed cycles | Load page-crossing extra cycles are checked; store page-crossing cycles remain fixed. |
| Indirect indexed cycles | `(indirect),Y` load page-crossing extra cycle is checked. |
| Bus behavior | RAM and expansion RAM writes work; Program ROM and Kernel ROM writes are ignored; UART `$D010/$D011` routes through the peripheral. |

These tests focus on the memory/addressing foundation needed by richer monitor
ROM code and future user programs. They are not a complete opcode behavior
suite.
