#!/usr/bin/env sh
set -eu

dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

printf '\251\117\215\020\320\251\113\215\020\320\352' > "$dir/ok_once.bin"
printf '\251\117\215\020\320\251\113\215\020\320\251\012\215\020\320\114\017\200' > "$dir/ok_loop.bin"
printf '\251\110\215\020\320\251\111\215\020\320\352' > "$dir/uart_hi.bin"
printf '\255\021\320\051\002\360\371\255\020\320\215\020\320\114\000\200' > "$dir/uart_echo.bin"
printf '\251\052\215\005\000\352\352' > "$dir/store_2a.bin"
printf '\251\101\215\020\320\352\251\102\215\020\320\114\013\200' > "$dir/breakpoint_test.bin"
python3 "$dir/build_echo_upper.py"

printf 'wrote %s\n' "$dir/ok_once.bin"
printf 'wrote %s\n' "$dir/ok_loop.bin"
printf 'wrote %s\n' "$dir/uart_hi.bin"
printf 'wrote %s\n' "$dir/uart_echo.bin"
printf 'wrote %s\n' "$dir/store_2a.bin"
printf 'wrote %s\n' "$dir/breakpoint_test.bin"
