#!/usr/bin/env sh
set -eu

tmpdir="${TMPDIR:-build/tests}"
mkdir -p "$tmpdir"
runner="$tmpdir/6502_echo_upper_runner"

programs/carts/build_carts.sh >/dev/null

cc -Wall -Wextra -pedantic -std=c99 -Isrc \
  -o "$runner" \
  programs/tests/echo_upper_runner.c \
  src/mem/mem.c src/cpu/cpu.c src/cpu/instructions.c \
  src/peripherals/uart.c -lm

"$runner"
