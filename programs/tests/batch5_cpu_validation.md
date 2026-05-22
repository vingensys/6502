# Batch 5 CPU Validation Tests

`cpu_batch5_runner.c` validates trap/interrupt and status-stack behavior without
starting ncurses. It compiles directly against the emulator CPU and memory
modules, loads tiny binaries at `$8000`, configures vectors through the memory
bus helpers, executes fixed instruction counts, and asserts PC, SP, stack bytes,
status flags, and interrupt cycles.

Run:

```sh
programs/tests/run_cpu_batch5.sh
```

Covered behavior:

| Area | Coverage |
|---|---|
| PHP | Pushed status has B set, unused bit 5 set, and preserves payload flags. |
| PLP | Restores C/Z/I/D/V/N, normalizes unused bit 5 to set, and clears internal B. |
| BRK | Pushes PC+2, pushes status with B and bit 5 set, sets I, jumps through `$FFFE/$FFFF`, and consumes 7 cycles. |
| RTI | Pulls status and PC low/high, restores PC, normalizes bit 5, and clears internal B. |
| IRQ | Ignored when I is set; serviced when I is clear through `$FFFE/$FFFF`. |
| NMI | Serviced through `$FFFA/$FFFB` even when I is set. |
| Interrupt stack status | IRQ/NMI push status with B clear and bit 5 set. |

This prepares the CPU core for future monitor breakpoints and interrupt-driven
devices. It does not implement monitor breakpoints.
