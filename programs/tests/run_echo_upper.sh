#!/usr/bin/env sh
set -eu

programs/carts/build_carts.sh >/dev/null

cc -Wall -Wextra -pedantic -std=c99 -Isrc \
  -o /tmp/6502_echo_upper_runner \
  programs/tests/echo_upper_runner.c \
  src/mem/mem.c src/cpu/cpu.c src/cpu/instructions.c \
  src/peripherals/uart.c -lm

/tmp/6502_echo_upper_runner
rm -f /tmp/6502_echo_upper_runner
