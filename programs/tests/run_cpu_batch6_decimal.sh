#!/usr/bin/env sh
set -eu

cc -Wall -Wextra -pedantic -std=c99 -Isrc \
  -o /tmp/6502_cpu_batch6_decimal_runner \
  programs/tests/cpu_batch6_decimal_runner.c \
  src/mem/mem.c \
  src/cpu/cpu.c \
  src/cpu/instructions.c \
  src/peripherals/uart.c \
  -lm

/tmp/6502_cpu_batch6_decimal_runner
