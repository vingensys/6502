#!/usr/bin/env sh
set -eu

tmpdir="${TMPDIR:-build/tests}"
mkdir -p "$tmpdir"
runner="$tmpdir/6502_monitor_polish_runner"

programs/monitor/build_monitor.sh >/dev/null
programs/carts/build_carts.sh >/dev/null

cc -Wall -Wextra -pedantic -std=c99 -Isrc \
  -o "$runner" \
  programs/tests/monitor_polish_runner.c \
  src/mem/mem.c src/cpu/cpu.c src/cpu/instructions.c \
  src/peripherals/uart.c -lm

"$runner"
