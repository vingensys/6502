#!/usr/bin/env sh
set -eu

programs/monitor/build_monitor.sh >/dev/null
programs/carts/build_carts.sh >/dev/null

cc -Wall -Wextra -pedantic -std=c99 -Isrc \
  -o /tmp/6502_monitor_breakpoint_runner \
  programs/tests/monitor_breakpoint_runner.c \
  src/mem/mem.c \
  src/cpu/cpu.c \
  src/cpu/instructions.c \
  src/peripherals/uart.c \
  -lm

/tmp/6502_monitor_breakpoint_runner
