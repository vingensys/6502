# Monitor Breakpoint Validation

`monitor_breakpoint_runner.c` validates the monitor-managed single breakpoint
without starting ncurses.

The runner:

1. Builds and loads `programs/monitor/monitor.bin` at `$E000`.
2. Builds and loads `programs/carts/breakpoint_test.bin` at `$8000`.
3. Queues monitor commands through UART input:
   - `B8005`
   - `L`
   - `G8000`
4. Runs the CPU until the monitor prints `BRK 8005`.
5. Verifies the original byte at `$8005` is restored.

Run:

```sh
programs/tests/run_monitor_breakpoint.sh
```
