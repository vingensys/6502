#!/usr/bin/env sh
set -eu

tmpdir="${TMPDIR:-build/tests}"
mkdir -p "$tmpdir"
runner="$tmpdir/6502_cpu_batch2_runner"
test_bin="$tmpdir/6502_cpu_batch2_test.bin"

cc -Wall -Wextra -pedantic -std=c99 -Isrc \
  -DTEST_BIN="\"$test_bin\"" \
  -o "$runner" \
  programs/tests/cpu_batch2_runner.c \
  src/mem/mem.c \
  src/cpu/cpu.c \
  src/cpu/instructions.c \
  src/peripherals/uart.c \
  -lm

"$runner"
