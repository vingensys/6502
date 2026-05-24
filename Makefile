CFLAGS	= -Wall -Wextra -pedantic -std=c99
LDFLAGS	= -L/usr/local/lib
LDLIBS	= -lm -lncurses

TERMUX_HOST ?=
TERMUX_USER ?=
TERMUX_PORT ?= 8022
TERMUX_REPO ?= ~/6502

TERMUX_SSH = ssh -p $(TERMUX_PORT) $(TERMUX_USER)@$(TERMUX_HOST)

sources = src/main.c src/mem/mem.c src/cpu/cpu.c src/cpu/instructions.c src/peripherals/interface.c src/peripherals/kinput.c src/peripherals/uart.c
headers = src/mem/mem.h src/cpu/cpu.h src/cpu/instructions.h src/peripherals/interface.h src/peripherals/kinput.h src/peripherals/uart.h src/utils/misc.h

.PHONY: all clean monitor carts os assets full test verify run-os run-monitor run-monitor-echo remote-shell remote-status remote-verify remote-run-os remote-run-monitor remote-run-monitor-echo

all: bin/emulator.out
	
bin/emulator.out: $(sources) $(headers)
	@mkdir -p bin
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(sources) $(LDLIBS)

clean:
	rm -rf bin

monitor:
	programs/monitor/build_monitor.sh

carts:
	programs/carts/build_carts.sh

os:
	cd programs/os/tms-os && make clean && make

assets: monitor carts os

full: clean all assets

test:
	programs/tests/run_cpu_batch6_decimal.sh
	programs/tests/run_cpu_batch5.sh
	programs/tests/run_cpu_batch4.sh
	programs/tests/run_cpu_batch3.sh
	programs/tests/run_cpu_batch2.sh

verify: full test
	git diff --check

run-os: all os
	./bin/emulator.out --rom programs/os/tms-os/build/tmsos.bin --ui headless

run-monitor: all monitor
	./bin/emulator.out --monitor --ui headless

run-monitor-echo: all monitor carts
	./bin/emulator.out --monitor --cart programs/carts/echo_upper.bin --ui headless

define require_termux_host
	@if [ -z "$(TERMUX_HOST)" ]; then \
		echo "TERMUX_HOST is required."; \
		echo "Usage:"; \
		echo "  make $@ TERMUX_HOST=<phone-ip> TERMUX_USER=<termux-user>"; \
		exit 2; \
	fi
	@if [ -z "$(TERMUX_USER)" ]; then \
		echo "TERMUX_USER is required."; \
		echo "Usage:"; \
		echo "  make $@ TERMUX_HOST=<phone-ip> TERMUX_USER=<termux-user>"; \
		exit 2; \
	fi
endef

remote-shell:
	$(call require_termux_host)
	$(TERMUX_SSH)

remote-status:
	$(call require_termux_host)
	$(TERMUX_SSH) 'cd $(TERMUX_REPO) && git status --short && uname -m && pwd'

remote-verify:
	$(call require_termux_host)
	$(TERMUX_SSH) 'cd $(TERMUX_REPO) && make verify'

remote-run-os:
	$(call require_termux_host)
	$(TERMUX_SSH) 'cd $(TERMUX_REPO) && make run-os'

remote-run-monitor:
	$(call require_termux_host)
	$(TERMUX_SSH) 'cd $(TERMUX_REPO) && make run-monitor'

remote-run-monitor-echo:
	$(call require_termux_host)
	$(TERMUX_SSH) 'cd $(TERMUX_REPO) && make run-monitor-echo'
