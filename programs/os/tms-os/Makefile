CA65 := ca65
CC65 := cc65
LD65 := ld65

BUILD := build
TARGET := $(BUILD)/tmsos.bin

ASFLAGS := --cpu 6502
CFLAGS := --cpu 6502 --target none -I include -O
LDFLAGS := -C linker.cfg
LDLIBS := none.lib

ASM_SRCS := src/crt0.s src/uart.s src/vectors.s
C_SRCS := src/kernel.c src/shell.c src/memory.c src/hex.c
OBJS := $(ASM_SRCS:src/%.s=$(BUILD)/%.o) $(C_SRCS:src/%.c=$(BUILD)/%.o)

.PHONY: all clean check-tools

all: check-tools $(TARGET)

check-tools:
	@command -v $(CA65) >/dev/null 2>&1 || { echo "ca65 not found; install cc65 (Termux: pkg install cc65)"; exit 1; }
	@command -v $(CC65) >/dev/null 2>&1 || { echo "cc65 not found; install cc65 (Termux: pkg install cc65)"; exit 1; }
	@command -v $(LD65) >/dev/null 2>&1 || { echo "ld65 not found; install cc65 (Termux: pkg install cc65)"; exit 1; }

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: src/%.s | $(BUILD)
	$(CA65) $(ASFLAGS) -o $@ $<

$(BUILD)/%.s: src/%.c | $(BUILD)
	$(CC65) $(CFLAGS) -o $@ $<

$(BUILD)/%.o: $(BUILD)/%.s
	$(CA65) $(ASFLAGS) -o $@ $<

$(TARGET): $(OBJS) linker.cfg
	$(LD65) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)
	@wc -c $@

clean:
	rm -rf $(BUILD)
