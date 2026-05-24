#!/usr/bin/env sh
set -eu

dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

printf '\251\117\215\020\320\251\113\215\020\320\352' > "$dir/ok_once.bin"
printf '\251\117\215\020\320\251\113\215\020\320\251\012\215\020\320\114\017\200' > "$dir/ok_loop.bin"
printf '\251\110\215\020\320\251\111\215\020\320\352' > "$dir/uart_hi.bin"
printf '\255\021\320\051\002\360\371\255\020\320\215\020\320\114\000\200' > "$dir/uart_echo.bin"
printf '\251\052\215\005\000\352\352' > "$dir/store_2a.bin"
printf '\251\101\215\020\320\352\251\102\215\020\320\114\013\200' > "$dir/breakpoint_test.bin"
printf '\251\021\205\200\251\200\205\201\040\011\341\040\014\341\040\000\341HELLO FROM OS ABI\000' > "$dir/os_call_demo.bin"
python3 "$dir/build_echo_upper.py"
(cd "$dir/c-sdk" && make clean && make)

printf 'wrote %s\n' "$dir/ok_once.bin"
printf 'wrote %s\n' "$dir/ok_loop.bin"
printf 'wrote %s\n' "$dir/uart_hi.bin"
printf 'wrote %s\n' "$dir/uart_echo.bin"
printf 'wrote %s\n' "$dir/store_2a.bin"
printf 'wrote %s\n' "$dir/breakpoint_test.bin"
printf 'wrote %s\n' "$dir/os_call_demo.bin"
printf 'wrote %s\n' "$dir/c-sdk/build/os_call_demo_c.bin"
