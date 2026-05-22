# CPU Variant/Profile Roadmap

This emulator currently targets the official NMOS 6502 instruction set. This
document records the intended path for supporting multiple 6502-family CPU
profiles without mixing incompatible opcode sets.

## Current Supported Profile

Current profile: official NMOS 6502.

Status after Batch 6A:

- Official opcode dispatch is complete for all 151 official NMOS 6502 opcodes.
- BRK, RTI, PHP, and PLP status semantics are hardened and regression-tested.
- A minimal CPU-level IRQ/NMI request path exists through `cpu_request_irq()`
  and `cpu_request_nmi()`.
- Decimal-mode ADC/SBC behavior is implemented and regression-tested.
- The remaining major official gap is deeper cycle validation.

Unofficial/illegal opcodes are not currently part of the supported CPU
contract.

## Remaining NMOS Completion Work

Before calling the official NMOS 6502 profile complete, finish:

- Full cycle validation across all official opcodes and relevant page-crossing
  cases.
- Optional external-device IRQ integration tests once peripherals can assert
  CPU interrupts directly.

## Proposed CLI Profiles

Future command-line profile selection should be explicit:

```sh
./bin/emulator.out --cpu nmos6502
./bin/emulator.out --cpu nmos6502-undoc
./bin/emulator.out --cpu 65c02
./bin/emulator.out --cpu nes2a03
```

Proposed profile meanings:

| Profile | Meaning |
| --- | --- |
| `nmos6502` | Official NMOS 6502 opcodes and behavior only. |
| `nmos6502-undoc` | NMOS 6502 plus undocumented/illegal opcode behavior. |
| `65c02` | WDC/Rockwell 65C02 official extensions and CMOS behavior changes. |
| `nes2a03` | NES RP2A03 CPU profile with NES-specific CPU/bus behavior. |

The default should remain `nmos6502` until another profile is explicitly
requested.

## Keep Profiles Separate

65C02 and undocumented NMOS opcode support must not be mixed casually.

Some opcode slots that are undocumented or illegal on NMOS 6502 were assigned
official meanings on CMOS 65C02 variants. Treating those slots as both
"undocumented NMOS" and "65C02 official" at the same time would produce a CPU
that does not match real hardware.

NES RP2A03 should also be separate. It is 6502-derived, but has its own
platform-specific CPU and bus behavior. In particular, NES work should not be
implemented as a simple "NMOS plus extras" mode once the emulator starts
modeling system behavior beyond core opcodes.

## Proposed Implementation Order

1. Finish official NMOS 6502.
2. Add CPU profile enum/config plumbing.
3. Add 65C02 official extensions.
4. Add optional NMOS undocumented opcode profile.
5. Add NES/RP2A03 profile later, only after the CPU and bus design is mature.

This order keeps the baseline CPU honest and gives future profiles a clean
place to attach profile-specific dispatch tables, status behavior, bus quirks,
and cycle rules.

## Monitor And OS Implications

The monitor ROM and any future tiny OS should initially target official NMOS
6502 only. That keeps boot, debugging, and test binaries portable across the
baseline emulator profile.

Programs that depend on 65C02 instructions, undocumented NMOS opcodes, or
NES/RP2A03 behavior should require explicit CPU profile selection. The emulator
should make that dependency visible rather than silently accepting code for the
wrong CPU family.
