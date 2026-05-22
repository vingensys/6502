# Batch 2 CPU Validation Tests

`cpu_batch2_runner.c` is a lightweight non-ncurses regression runner for CPU
control-flow behavior. It compiles directly against the emulator CPU and memory
modules, writes tiny test binaries to `/tmp`, loads them at `$8000`, executes a
fixed number of instructions, and checks CPU registers, memory, PC, SP, stack
bytes, and branch cycle counts.

Run:

```sh
programs/tests/run_cpu_batch2.sh
```

Covered behavior:

| Area | Coverage |
|---|---|
| JSR / RTS | Return address bytes pushed to `$01FD/$01FC`, PC enters subroutine, RTS resumes after JSR, SP restored. |
| Nested JSR / RTS | Two nested calls return to caller and preserve final result. |
| PHA / PLA | PHA writes to stack page `$0100-$01FF`, decrements SP, PLA restores A and SP. |
| PHP / PLP | PHP pushes status to stack page, PLP restores Carry and SP. Status bit 5/B normalization remains a known partial area. |
| TSX / TXS | TXS copies X to SP; TSX copies SP back to X. |
| Branches | BEQ, BNE, BCC, BCS, BMI, BPL, BVC, BVS taken and not-taken PC behavior. |
| Relative offsets | Forward and backward signed branch offsets. |
| Branch cycles | Not-taken branch = 2 cycles, taken same-page branch = 3 cycles, taken page-crossing branch = 4 cycles. |

These tests validate the core behavior needed by monitor ROM subroutines and
future monitor/OS work. They are not a full cycle-accuracy certification.
