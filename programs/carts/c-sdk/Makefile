CA65 := ca65
CC65 := cc65
LD65 := ld65

BUILD := build
DEMO_TARGET := $(BUILD)/os_call_demo_c.bin
DEMO_MAP := $(BUILD)/os_call_demo_c.map
CALC_TARGET := $(BUILD)/tms_calc.bin
CALC_MAP := $(BUILD)/tms_calc.map

ASFLAGS := --cpu 6502
CFLAGS := --cpu 6502 --target none -I include -O
LDFLAGS := -C linker_cart.cfg
LDLIBS := none.lib

ASM_SRCS := src/crt0_cart.s src/tmsos_cart.s
COMMON_OBJS := $(ASM_SRCS:src/%.s=$(BUILD)/%.o)
DEMO_OBJS := $(COMMON_OBJS) $(BUILD)/main.o
CALC_OBJS := $(COMMON_OBJS) $(BUILD)/calc_main.o

.PHONY: all clean check-tools
.INTERMEDIATE: $(BUILD)/main.s $(BUILD)/calc_main.s

all: check-tools $(DEMO_TARGET) $(CALC_TARGET)

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

$(BUILD)/calc_main.s: examples/calc/main.c | $(BUILD)
	$(CC65) $(CFLAGS) -o $@ $<

$(BUILD)/%.o: $(BUILD)/%.s
	$(CA65) $(ASFLAGS) -o $@ $<

$(DEMO_TARGET): $(DEMO_OBJS) linker_cart.cfg
	$(LD65) $(LDFLAGS) -m $(DEMO_MAP) -o $@ $(DEMO_OBJS) $(LDLIBS)
	@wc -c $@

$(CALC_TARGET): $(CALC_OBJS) linker_cart.cfg
	$(LD65) $(LDFLAGS) -m $(CALC_MAP) -o $@ $(CALC_OBJS) $(LDLIBS)
	@wc -c $@

clean:
	rm -rf $(BUILD)
