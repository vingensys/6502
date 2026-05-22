# 6502 Opcode Coverage Audit

This audit covers official NMOS 6502 opcodes only. Unofficial/illegal opcodes are intentionally excluded from the table.

## Summary

| Metric | Count |
|---|---:|
| Official NMOS 6502 opcodes | 151 |
| Implemented and likely usable | 151 |
| Missing from dispatch | 0 |
| Partial | 0 |
| Suspect | 0 |

All 151 official opcodes have a non-`XXX` dispatch entry in `src/cpu/instructions.c`. Batch 1 fixed the previously known high-impact basic correctness issues, so no official opcode is currently classified as `suspect` by this audit. `partial` would mean the opcode has a handler but known 6502 behavior is incomplete; no official opcode is currently classified that way after Batch 6A.

Unsupported/unofficial opcode policy: there is no formal policy yet. The lookup table contains many unofficial opcode slots mapped to `XXX` stubs or unofficial no-op behavior, but those are outside this audit and should not be treated as supported CPU behavior.

Cycle note: the lookup table contains base cycle counts and some address modes report page-crossing additions. The emulator reports total cycles, but this audit does not certify cycle accuracy.

## Batch 1 Fixes Applied

- `cpu_mod_sr()` now allows intentional modification of Carry flag bit 0 while still rejecting unused bit 5.
- ORA and EOR now update Z/N and leave Carry unchanged.
- BIT now sets Z from the full `A & M` result and sets N/V from memory bits 7/6.
- DEX now decrements X with uint8_t wraparound and updates Z/N.
- Official NOP `$EA` no longer advances PC beyond the opcode fetch.
- Manual smoke-test byte sequences are documented in `programs/tests/batch1_cpu_smoke_tests.md`.

## Batch 2 Validation Added

- `programs/tests/run_cpu_batch2.sh` builds and runs a lightweight non-ncurses CPU regression runner.
- JSR/RTS validation covers pushed return address bytes, PC target, return-to-caller behavior, nested subroutines, and SP restoration.
- Stack validation covers PHA/PLA data movement through `$0100-$01FF`, SP pre/post behavior, PHP/PLP Carry restore, and TSX/TXS.
- Branch validation covers BEQ, BNE, BCC, BCS, BMI, BPL, BVC, and BVS taken/not-taken behavior.
- Relative branch validation covers forward and backward signed offsets.
- Branch cycle validation covers not-taken branches, taken same-page branches, and taken page-crossing branches.
- PHP/PLP stack behavior was initially covered here; Batch 5 later hardens full status normalization.

## Batch 3 Validation Added

- `programs/tests/run_cpu_batch3.sh` builds and runs a lightweight non-ncurses CPU regression runner for load/store addressing and bus behavior.
- LDA, LDX, and LDY addressing modes are validated for correct loaded values and Z/N flag updates.
- STA, STX, and STY addressing modes are validated for effective write address, preserved value, and no SR mutation.
- Zero-page indexed wraparound is validated for zero page,X and zero page,Y.
- Zero-page indirect pointer behavior is validated for `(indirect,X)` and `(indirect),Y`, including high-byte wrap from `$FF` to `$00`.
- Absolute indexed and `(indirect),Y` load page-crossing extra cycles are validated.
- Absolute indexed store cycles are validated as fixed-cycle stores.
- Bus behavior is validated for RAM, expansion RAM, Program ROM write-ignore, Kernel ROM write-ignore, and UART `$D010/$D011` routing.

## Batch 4 Validation Added

- `programs/tests/run_cpu_batch4.sh` builds and runs a lightweight non-ncurses CPU regression runner for ALU and flag behavior.
- AND, ORA, and EOR are validated for accumulator result, Z/N flag behavior, and C/V preservation across representative immediate, zero-page, absolute, indexed, and indirect modes.
- CMP, CPX, and CPY are validated for equal, greater-than, and less-than cases, including C, Z, and N behavior.
- ADC and SBC are validated in binary mode for carry/borrow, zero, negative, and overflow behavior with decimal mode clear.
- ASL, LSR, ROL, and ROR are validated in accumulator and memory forms, including incoming Carry for rotates and outgoing Carry from shifted/rotated-out bits.
- INC and DEC memory forms are validated for zero, negative, and wraparound results.
- Decimal-mode BCD arithmetic was added and validated in Batch 6A.

## Batch 5 Validation Added

- `programs/tests/run_cpu_batch5.sh` builds and runs a lightweight non-ncurses CPU regression runner for traps, interrupts, and status-stack behavior.
- PHP pushes status with Break and unused bit 5 set, while preserving the meaningful payload flags.
- PLP and RTI restore status with unused bit 5 normalized set and internal Break normalized clear.
- BRK pushes the correct PC+2 return address, pushes status with Break and bit 5 set, sets Interrupt Disable, jumps through `$FFFE/$FFFF`, and accounts for 7 cycles.
- RTI pulls status and PC low/high and resumes at the stacked return address.
- Minimal external IRQ/NMI request handling is validated: IRQ is masked by I, NMI is not, both push status with Break clear and bit 5 set, set I, and load the correct vector.

## Batch 6A Decimal Validation Added

- `programs/tests/run_cpu_batch6_decimal.sh` builds and runs a lightweight non-ncurses CPU regression runner for decimal-mode ADC/SBC.
- ADC decimal mode is validated for packed-BCD addition, incoming Carry, decimal carry out, zero result, and bit-7 result behavior.
- SBC decimal mode is validated for packed-BCD subtraction, no-borrow Carry set, borrow Carry clear, and incoming borrow behavior.
- ADC/SBC decimal behavior is covered across immediate, zero-page, absolute, absolute indexed, and representative indirect addressing modes.
- V in decimal mode follows the binary overflow calculation from the unadjusted operation; C/Z/N are based on the decimal-adjusted result.

## Full Official Opcode Table

| Opcode | Mnemonic | Addressing mode | Bytes | Expected cycles | Implemented | Current handler/function | Notes |
|---:|---|---|---:|---:|---|---|---|
| `$00` | BRK | implied | 1 | 7 | yes | `BRK + IMP` | Batch 5 validates PC+2 stack return, status push with B/bit 5 set, I set, IRQ/BRK vector load, and 7-cycle accounting. |
| `$01` | ORA | (indirect,X) | 2 | 6 | yes | `ORA + IZX` | Batch 4 validates logical result, Z/N flags, and C/V preservation. Batch 3 validates this addressing helper through load/store tests. |
| `$05` | ORA | zero page | 2 | 3 | yes | `ORA + ZP0` | Batch 4 validates logical result, Z/N flags, and C/V preservation. Batch 3 validates this addressing helper through load/store tests. |
| `$06` | ASL | zero page | 2 | 5 | yes | `ASL + ZP0` | Batch 4 validates accumulator/memory result plus C/Z/N flag behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$08` | PHP | implied | 1 | 3 | yes | `PHP + IMP` | Batch 5 validates pushed status has B and bit 5 set while preserving payload flags. |
| `$09` | ORA | immediate | 2 | 2 | yes | `ORA + IMM` | Batch 4 validates logical result, Z/N flags, and C/V preservation. |
| `$0A` | ASL | accumulator | 1 | 2 | yes | `ASL + IMP` | Batch 4 validates accumulator/memory result plus C/Z/N flag behavior. |
| `$0D` | ORA | absolute | 3 | 4 | yes | `ORA + ABS` | Batch 4 validates logical result, Z/N flags, and C/V preservation. Batch 3 validates this addressing helper through load/store tests. |
| `$0E` | ASL | absolute | 3 | 6 | yes | `ASL + ABS` | Batch 4 validates accumulator/memory result plus C/Z/N flag behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$10` | BPL | relative | 2 | 2 | yes | `BPL + REL` | Batch 2 validates taken/not-taken PC behavior. |
| `$11` | ORA | (indirect),Y | 2 | 5 | yes | `ORA + IZY` | Batch 4 validates logical result, Z/N flags, and C/V preservation. Batch 3 validates this addressing helper through load/store tests. |
| `$15` | ORA | zero page,X | 2 | 4 | yes | `ORA + ZPX` | Batch 4 validates logical result, Z/N flags, and C/V preservation. Batch 3 validates this addressing helper through load/store tests. |
| `$16` | ASL | zero page,X | 2 | 6 | yes | `ASL + ZPX` | Batch 4 validates accumulator/memory result plus C/Z/N flag behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$18` | CLC | implied | 1 | 2 | yes | `CLC + IMP` | Batch 1 fixed Carry mutation; Batch 2 validates Carry-dependent branches. |
| `$19` | ORA | absolute,Y | 3 | 4 | yes | `ORA + ABY` | Batch 4 validates logical result, Z/N flags, and C/V preservation. Batch 3 validates this addressing helper through load/store tests. |
| `$1D` | ORA | absolute,X | 3 | 4 | yes | `ORA + ABX` | Batch 4 validates logical result, Z/N flags, and C/V preservation. Batch 3 validates this addressing helper through load/store tests. |
| `$1E` | ASL | absolute,X | 3 | 7 | yes | `ASL + ABX` | Batch 4 validates accumulator/memory result plus C/Z/N flag behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$20` | JSR | absolute | 3 | 6 | yes | `JSR + ABS` | Batch 2 validates return address push, PC target, nested calls, and SP behavior. |
| `$21` | AND | (indirect,X) | 2 | 6 | yes | `AND + IZX` | Batch 4 validates logical result, Z/N flags, and C/V preservation. Batch 3 validates this addressing helper through load/store tests. |
| `$24` | BIT | zero page | 2 | 3 | yes | `BIT + ZP0` | Batch 1 corrected Z/V/N behavior; Batch 2 uses BIT to validate V branches. |
| `$25` | AND | zero page | 2 | 3 | yes | `AND + ZP0` | Batch 4 validates logical result, Z/N flags, and C/V preservation. Batch 3 validates this addressing helper through load/store tests. |
| `$26` | ROL | zero page | 2 | 5 | yes | `ROL + ZP0` | Batch 4 validates accumulator/memory result plus C/Z/N flag behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$28` | PLP | implied | 1 | 4 | yes | `PLP + IMP` | Batch 5 validates status restore for C/Z/I/D/V/N with bit 5 set and internal B clear. |
| `$29` | AND | immediate | 2 | 2 | yes | `AND + IMM` | Batch 4 validates logical result, Z/N flags, and C/V preservation. |
| `$2A` | ROL | accumulator | 1 | 2 | yes | `ROL + IMP` | Batch 4 validates accumulator/memory result plus C/Z/N flag behavior. |
| `$2C` | BIT | absolute | 3 | 4 | yes | `BIT + ABS` | Batch 1 corrected Z/V/N behavior. |
| `$2D` | AND | absolute | 3 | 4 | yes | `AND + ABS` | Batch 4 validates logical result, Z/N flags, and C/V preservation. Batch 3 validates this addressing helper through load/store tests. |
| `$2E` | ROL | absolute | 3 | 6 | yes | `ROL + ABS` | Batch 4 validates accumulator/memory result plus C/Z/N flag behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$30` | BMI | relative | 2 | 2 | yes | `BMI + REL` | Batch 2 validates taken/not-taken PC behavior. |
| `$31` | AND | (indirect),Y | 2 | 5 | yes | `AND + IZY` | Batch 4 validates logical result, Z/N flags, and C/V preservation. Batch 3 validates this addressing helper through load/store tests. |
| `$35` | AND | zero page,X | 2 | 4 | yes | `AND + ZPX` | Batch 4 validates logical result, Z/N flags, and C/V preservation. Batch 3 validates this addressing helper through load/store tests. |
| `$36` | ROL | zero page,X | 2 | 6 | yes | `ROL + ZPX` | Batch 4 validates accumulator/memory result plus C/Z/N flag behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$38` | SEC | implied | 1 | 2 | yes | `SEC + IMP` | Batch 1 fixed Carry mutation; Batch 2 validates Carry-dependent branches. |
| `$39` | AND | absolute,Y | 3 | 4 | yes | `AND + ABY` | Batch 4 validates logical result, Z/N flags, and C/V preservation. Batch 3 validates this addressing helper through load/store tests. |
| `$3D` | AND | absolute,X | 3 | 4 | yes | `AND + ABX` | Batch 4 validates logical result, Z/N flags, and C/V preservation. Batch 3 validates this addressing helper through load/store tests. |
| `$3E` | ROL | absolute,X | 3 | 7 | yes | `ROL + ABX` | Batch 4 validates accumulator/memory result plus C/Z/N flag behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$40` | RTI | implied | 1 | 6 | yes | `RTI + IMP` | Batch 5 validates status pull, PC low/high pull, return target, bit 5 normalization, and internal B clear. |
| `$41` | EOR | (indirect,X) | 2 | 6 | yes | `EOR + IZX` | Batch 4 validates logical result, Z/N flags, and C/V preservation. Batch 3 validates this addressing helper through load/store tests. |
| `$45` | EOR | zero page | 2 | 3 | yes | `EOR + ZP0` | Batch 4 validates logical result, Z/N flags, and C/V preservation. Batch 3 validates this addressing helper through load/store tests. |
| `$46` | LSR | zero page | 2 | 5 | yes | `LSR + ZP0` | Batch 4 validates accumulator/memory result plus C/Z/N flag behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$48` | PHA | implied | 1 | 3 | yes | `PHA + IMP` | Batch 2 validates stack page write and SP decrement. |
| `$49` | EOR | immediate | 2 | 2 | yes | `EOR + IMM` | Batch 4 validates logical result, Z/N flags, and C/V preservation. |
| `$4A` | LSR | accumulator | 1 | 2 | yes | `LSR + IMP` | Batch 4 validates accumulator/memory result plus C/Z/N flag behavior. |
| `$4C` | JMP | absolute | 3 | 3 | yes | `JMP + ABS` | Absolute jump handler present. |
| `$4D` | EOR | absolute | 3 | 4 | yes | `EOR + ABS` | Batch 4 validates logical result, Z/N flags, and C/V preservation. Batch 3 validates this addressing helper through load/store tests. |
| `$4E` | LSR | absolute | 3 | 6 | yes | `LSR + ABS` | Batch 4 validates accumulator/memory result plus C/Z/N flag behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$50` | BVC | relative | 2 | 2 | yes | `BVC + REL` | Batch 2 validates taken/not-taken PC behavior. |
| `$51` | EOR | (indirect),Y | 2 | 5 | yes | `EOR + IZY` | Batch 4 validates logical result, Z/N flags, and C/V preservation. Batch 3 validates this addressing helper through load/store tests. |
| `$55` | EOR | zero page,X | 2 | 4 | yes | `EOR + ZPX` | Batch 4 validates logical result, Z/N flags, and C/V preservation. Batch 3 validates this addressing helper through load/store tests. |
| `$56` | LSR | zero page,X | 2 | 6 | yes | `LSR + ZPX` | Batch 4 validates accumulator/memory result plus C/Z/N flag behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$58` | CLI | implied | 1 | 2 | yes | `CLI + IMP` |  |
| `$59` | EOR | absolute,Y | 3 | 4 | yes | `EOR + ABY` | Batch 4 validates logical result, Z/N flags, and C/V preservation. Batch 3 validates this addressing helper through load/store tests. |
| `$5D` | EOR | absolute,X | 3 | 4 | yes | `EOR + ABX` | Batch 4 validates logical result, Z/N flags, and C/V preservation. Batch 3 validates this addressing helper through load/store tests. |
| `$5E` | LSR | absolute,X | 3 | 7 | yes | `LSR + ABX` | Batch 4 validates accumulator/memory result plus C/Z/N flag behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$60` | RTS | implied | 1 | 6 | yes | `RTS + IMP` | Batch 2 validates return-to-caller behavior, nested returns, and SP restoration. |
| `$61` | ADC | (indirect,X) | 2 | 6 | yes | `ADC + IZX` | Batch 6A validates decimal ADC result and C/Z/N behavior; V uses binary overflow from the unadjusted operation. Batch 4 validates binary ADC behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$65` | ADC | zero page | 2 | 3 | yes | `ADC + ZP0` | Batch 6A validates decimal ADC result and C/Z/N behavior; V uses binary overflow from the unadjusted operation. Batch 4 validates binary ADC behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$66` | ROR | zero page | 2 | 5 | yes | `ROR + ZP0` | Batch 4 validates accumulator/memory result plus C/Z/N flag behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$68` | PLA | implied | 1 | 4 | yes | `PLA + IMP` | Batch 2 validates stack pull, A restore, and SP increment. |
| `$69` | ADC | immediate | 2 | 2 | yes | `ADC + IMM` | Batch 6A validates decimal ADC result and C/Z/N behavior; V uses binary overflow from the unadjusted operation. Batch 4 validates binary ADC behavior. |
| `$6A` | ROR | accumulator | 1 | 2 | yes | `ROR + IMP` | Batch 4 validates accumulator/memory result plus C/Z/N flag behavior. |
| `$6C` | JMP | indirect | 3 | 5 | yes | `JMP + IND` | Indirect form attempts NMOS page-wrap bug behavior. |
| `$6D` | ADC | absolute | 3 | 4 | yes | `ADC + ABS` | Batch 6A validates decimal ADC result and C/Z/N behavior; V uses binary overflow from the unadjusted operation. Batch 4 validates binary ADC behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$6E` | ROR | absolute | 3 | 6 | yes | `ROR + ABS` | Batch 4 validates accumulator/memory result plus C/Z/N flag behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$70` | BVS | relative | 2 | 2 | yes | `BVS + REL` | Batch 2 validates taken/not-taken PC behavior. |
| `$71` | ADC | (indirect),Y | 2 | 5 | yes | `ADC + IZY` | Batch 6A validates decimal ADC result and C/Z/N behavior; V uses binary overflow from the unadjusted operation. Batch 4 validates binary ADC behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$75` | ADC | zero page,X | 2 | 4 | yes | `ADC + ZPX` | Batch 6A validates decimal ADC result and C/Z/N behavior; V uses binary overflow from the unadjusted operation. Batch 4 validates binary ADC behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$76` | ROR | zero page,X | 2 | 6 | yes | `ROR + ZPX` | Batch 4 validates accumulator/memory result plus C/Z/N flag behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$78` | SEI | implied | 1 | 2 | yes | `SEI + IMP` |  |
| `$79` | ADC | absolute,Y | 3 | 4 | yes | `ADC + ABY` | Batch 6A validates decimal ADC result and C/Z/N behavior; V uses binary overflow from the unadjusted operation. Batch 4 validates binary ADC behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$7D` | ADC | absolute,X | 3 | 4 | yes | `ADC + ABX` | Batch 6A validates decimal ADC result and C/Z/N behavior; V uses binary overflow from the unadjusted operation. Batch 4 validates binary ADC behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$7E` | ROR | absolute,X | 3 | 7 | yes | `ROR + ABX` | Batch 4 validates accumulator/memory result plus C/Z/N flag behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$81` | STA | (indirect,X) | 2 | 6 | yes | `STA + IZX` | Batch 3 validates effective address, preserved value, and SR preservation. |
| `$84` | STY | zero page | 2 | 3 | yes | `STY + ZP0` | Batch 3 validates effective address, preserved value, and SR preservation. |
| `$85` | STA | zero page | 2 | 3 | yes | `STA + ZP0` | Batch 3 validates effective address, preserved value, and SR preservation. |
| `$86` | STX | zero page | 2 | 3 | yes | `STX + ZP0` | Batch 3 validates effective address, preserved value, and SR preservation. |
| `$88` | DEY | implied | 1 | 2 | yes | `DEY + IMP` |  |
| `$8A` | TXA | implied | 1 | 2 | yes | `TXA + IMP` |  |
| `$8C` | STY | absolute | 3 | 4 | yes | `STY + ABS` | Batch 3 validates effective address, preserved value, SR preservation, and bus writes. |
| `$8D` | STA | absolute | 3 | 4 | yes | `STA + ABS` | Batch 3 validates effective address, preserved value, SR preservation, and bus writes. |
| `$8E` | STX | absolute | 3 | 4 | yes | `STX + ABS` | Batch 3 validates effective address, preserved value, SR preservation, and bus writes. |
| `$90` | BCC | relative | 2 | 2 | yes | `BCC + REL` | Batch 2 validates taken/not-taken PC behavior. |
| `$91` | STA | (indirect),Y | 2 | 6 | yes | `STA + IZY` | Batch 3 validates pointer wrap, effective address, preserved value, and SR preservation. |
| `$94` | STY | zero page,X | 2 | 4 | yes | `STY + ZPX` | Batch 3 validates zero-page wraparound, preserved value, and SR preservation. |
| `$95` | STA | zero page,X | 2 | 4 | yes | `STA + ZPX` | Batch 3 validates zero-page wraparound, preserved value, and SR preservation. |
| `$96` | STX | zero page,Y | 2 | 4 | yes | `STX + ZPY` | Batch 3 validates zero-page wraparound, preserved value, and SR preservation. |
| `$98` | TYA | implied | 1 | 2 | yes | `TYA + IMP` |  |
| `$99` | STA | absolute,Y | 3 | 5 | yes | `STA + ABY` | Batch 3 validates page-cross effective address, fixed store cycles, preserved value, and SR preservation. |
| `$9A` | TXS | implied | 1 | 2 | yes | `TXS + IMP` | Batch 2 validates X-to-SP transfer. |
| `$9D` | STA | absolute,X | 3 | 5 | yes | `STA + ABX` | Batch 3 validates page-cross effective address, fixed store cycles, preserved value, and SR preservation. |
| `$A0` | LDY | immediate | 2 | 2 | yes | `LDY + IMM` | Batch 3 validates loaded value and Z/N flags. |
| `$A1` | LDA | (indirect,X) | 2 | 6 | yes | `LDA + IZX` | Batch 3 validates pointer wrap, loaded value, and Z/N flags. |
| `$A2` | LDX | immediate | 2 | 2 | yes | `LDX + IMM` | Batch 3 validates loaded value and Z/N flags. |
| `$A4` | LDY | zero page | 2 | 3 | yes | `LDY + ZP0` | Batch 3 validates loaded value and Z/N flags. |
| `$A5` | LDA | zero page | 2 | 3 | yes | `LDA + ZP0` | Batch 3 validates loaded value and Z/N flags. |
| `$A6` | LDX | zero page | 2 | 3 | yes | `LDX + ZP0` | Batch 3 validates loaded value and Z/N flags. |
| `$A8` | TAY | implied | 1 | 2 | yes | `TAY + IMP` |  |
| `$A9` | LDA | immediate | 2 | 2 | yes | `LDA + IMM` |  |
| `$AA` | TAX | implied | 1 | 2 | yes | `TAX + IMP` |  |
| `$AC` | LDY | absolute | 3 | 4 | yes | `LDY + ABS` | Batch 3 validates loaded value and Z/N flags. |
| `$AD` | LDA | absolute | 3 | 4 | yes | `LDA + ABS` | Batch 3 validates loaded value and Z/N flags. |
| `$AE` | LDX | absolute | 3 | 4 | yes | `LDX + ABS` | Batch 3 validates loaded value and Z/N flags. |
| `$B0` | BCS | relative | 2 | 2 | yes | `BCS + REL` | Batch 2 validates taken/not-taken PC behavior. |
| `$B1` | LDA | (indirect),Y | 2 | 5 | yes | `LDA + IZY` | Batch 3 validates pointer high-byte wrap, page-cross cycle, loaded value, and Z/N flags. |
| `$B4` | LDY | zero page,X | 2 | 4 | yes | `LDY + ZPX` | Batch 3 validates zero-page wraparound, loaded value, and Z/N flags. |
| `$B5` | LDA | zero page,X | 2 | 4 | yes | `LDA + ZPX` | Batch 3 validates zero-page wraparound, loaded value, and Z/N flags. |
| `$B6` | LDX | zero page,Y | 2 | 4 | yes | `LDX + ZPY` | Batch 3 validates zero-page wraparound, loaded value, and Z/N flags. |
| `$B8` | CLV | implied | 1 | 2 | yes | `CLV + IMP` |  |
| `$B9` | LDA | absolute,Y | 3 | 4 | yes | `LDA + ABY` | Batch 3 validates effective address, page-cross cycle, loaded value, and Z/N flags. |
| `$BA` | TSX | implied | 1 | 2 | yes | `TSX + IMP` | Batch 2 validates SP-to-X transfer. |
| `$BC` | LDY | absolute,X | 3 | 4 | yes | `LDY + ABX` | Batch 3 validates effective address, page-cross cycle, loaded value, and Z/N flags. |
| `$BD` | LDA | absolute,X | 3 | 4 | yes | `LDA + ABX` | Batch 3 validates effective address, page-cross cycle, loaded value, and Z/N flags. |
| `$BE` | LDX | absolute,Y | 3 | 4 | yes | `LDX + ABY` | Batch 3 validates effective address, page-cross cycle, loaded value, and Z/N flags. |
| `$C0` | CPY | immediate | 2 | 2 | yes | `CPY + IMM` | Batch 4 validates equal, greater-than, and less-than C/Z/N behavior. |
| `$C1` | CMP | (indirect,X) | 2 | 6 | yes | `CMP + IZX` | Batch 4 validates equal, greater-than, and less-than C/Z/N behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$C4` | CPY | zero page | 2 | 3 | yes | `CPY + ZP0` | Batch 4 validates equal, greater-than, and less-than C/Z/N behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$C5` | CMP | zero page | 2 | 3 | yes | `CMP + ZP0` | Batch 4 validates equal, greater-than, and less-than C/Z/N behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$C6` | DEC | zero page | 2 | 5 | yes | `DEC + ZP0` | Batch 4 validates memory result, wraparound, and Z/N flag behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$C8` | INY | implied | 1 | 2 | yes | `INY + IMP` |  |
| `$C9` | CMP | immediate | 2 | 2 | yes | `CMP + IMM` | Batch 4 validates equal, greater-than, and less-than C/Z/N behavior. |
| `$CA` | DEX | implied | 1 | 2 | yes | `DEX + IMP` | Batch 1 corrected decrement behavior; Batch 1 smoke tests cover wrap to `$FF` and N/Z flags. |
| `$CC` | CPY | absolute | 3 | 4 | yes | `CPY + ABS` | Batch 4 validates equal, greater-than, and less-than C/Z/N behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$CD` | CMP | absolute | 3 | 4 | yes | `CMP + ABS` | Batch 4 validates equal, greater-than, and less-than C/Z/N behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$CE` | DEC | absolute | 3 | 6 | yes | `DEC + ABS` | Batch 4 validates memory result, wraparound, and Z/N flag behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$D0` | BNE | relative | 2 | 2 | yes | `BNE + REL` | Batch 2 validates taken/not-taken PC behavior and backward signed offset. |
| `$D1` | CMP | (indirect),Y | 2 | 5 | yes | `CMP + IZY` | Batch 4 validates equal, greater-than, and less-than C/Z/N behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$D5` | CMP | zero page,X | 2 | 4 | yes | `CMP + ZPX` | Batch 4 validates equal, greater-than, and less-than C/Z/N behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$D6` | DEC | zero page,X | 2 | 6 | yes | `DEC + ZPX` | Batch 4 validates memory result, wraparound, and Z/N flag behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$D8` | CLD | implied | 1 | 2 | yes | `CLD + IMP` | Clears decimal flag; decimal arithmetic is otherwise unsupported. |
| `$D9` | CMP | absolute,Y | 3 | 4 | yes | `CMP + ABY` | Batch 4 validates equal, greater-than, and less-than C/Z/N behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$DD` | CMP | absolute,X | 3 | 4 | yes | `CMP + ABX` | Batch 4 validates equal, greater-than, and less-than C/Z/N behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$DE` | DEC | absolute,X | 3 | 7 | yes | `DEC + ABX` | Batch 4 validates memory result, wraparound, and Z/N flag behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$E0` | CPX | immediate | 2 | 2 | yes | `CPX + IMM` | Batch 4 validates equal, greater-than, and less-than C/Z/N behavior. |
| `$E1` | SBC | (indirect,X) | 2 | 6 | yes | `SBC + IZX` | Batch 6A validates decimal SBC result and C/Z/N behavior; V uses binary overflow from the unadjusted operation. Batch 4 validates binary SBC behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$E4` | CPX | zero page | 2 | 3 | yes | `CPX + ZP0` | Batch 4 validates equal, greater-than, and less-than C/Z/N behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$E5` | SBC | zero page | 2 | 3 | yes | `SBC + ZP0` | Batch 6A validates decimal SBC result and C/Z/N behavior; V uses binary overflow from the unadjusted operation. Batch 4 validates binary SBC behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$E6` | INC | zero page | 2 | 5 | yes | `INC + ZP0` | Batch 4 validates memory result, wraparound, and Z/N flag behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$E8` | INX | implied | 1 | 2 | yes | `INX + IMP` |  |
| `$E9` | SBC | immediate | 2 | 2 | yes | `SBC + IMM` | Batch 6A validates decimal SBC result and C/Z/N behavior; V uses binary overflow from the unadjusted operation. Batch 4 validates binary SBC behavior. |
| `$EA` | NOP | implied | 1 | 2 | yes | `NOP + IMP` | Batch 1 corrected PC advancement behavior. |
| `$EC` | CPX | absolute | 3 | 4 | yes | `CPX + ABS` | Batch 4 validates equal, greater-than, and less-than C/Z/N behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$ED` | SBC | absolute | 3 | 4 | yes | `SBC + ABS` | Batch 6A validates decimal SBC result and C/Z/N behavior; V uses binary overflow from the unadjusted operation. Batch 4 validates binary SBC behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$EE` | INC | absolute | 3 | 6 | yes | `INC + ABS` | Batch 4 validates memory result, wraparound, and Z/N flag behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$F0` | BEQ | relative | 2 | 2 | yes | `BEQ + REL` | Batch 2 validates taken/not-taken PC behavior and page-cross cycles. |
| `$F1` | SBC | (indirect),Y | 2 | 5 | yes | `SBC + IZY` | Batch 6A validates decimal SBC result and C/Z/N behavior; V uses binary overflow from the unadjusted operation. Batch 4 validates binary SBC behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$F5` | SBC | zero page,X | 2 | 4 | yes | `SBC + ZPX` | Batch 6A validates decimal SBC result and C/Z/N behavior; V uses binary overflow from the unadjusted operation. Batch 4 validates binary SBC behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$F6` | INC | zero page,X | 2 | 6 | yes | `INC + ZPX` | Batch 4 validates memory result, wraparound, and Z/N flag behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$F8` | SED | implied | 1 | 2 | yes | `SED + IMP` | Sets decimal flag, but ADC/SBC do not implement BCD arithmetic. |
| `$F9` | SBC | absolute,Y | 3 | 4 | yes | `SBC + ABY` | Batch 6A validates decimal SBC result and C/Z/N behavior; V uses binary overflow from the unadjusted operation. Batch 4 validates binary SBC behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$FD` | SBC | absolute,X | 3 | 4 | yes | `SBC + ABX` | Batch 6A validates decimal SBC result and C/Z/N behavior; V uses binary overflow from the unadjusted operation. Batch 4 validates binary SBC behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |
| `$FE` | INC | absolute,X | 3 | 7 | yes | `INC + ABX` | Batch 4 validates memory result, wraparound, and Z/N flag behavior. Effective-address helpers are covered by Batch 3 load/store tests where shared. |

## Major Missing Groups

No official opcode group is completely absent from the dispatch table. The remaining gaps are deeper cycle accuracy, device-driven interrupt integration, and broader automated regression coverage rather than dispatch presence.

- Stack operations: PHA/PLA, PHP/PLP, and TSX/TXS are validated, including status bit 5 and Break-bit normalization for PHP/PLP.
- Subroutines: JSR/RTS and nested JSR/RTS behavior are validated by Batch 2 tests.
- Branches: all official branches have taken/not-taken validation; BNE has backward signed-offset validation; BEQ has page-cross cycle validation.
- Shifts/rotates: accumulator and memory forms are validated for ASL, LSR, ROL, and ROR, including Carry behavior and Z/N flags.
- Comparisons: CMP/CPX/CPY equal, greater-than, and less-than cases are validated for C/Z/N behavior.
- Interrupts: BRK/RTI and a minimal external IRQ/NMI request path are validated. No peripherals currently assert IRQ/NMI automatically.
- Decimal mode: SED/CLD and ADC/SBC BCD arithmetic are implemented and validated by Batch 6A.
- Indexed/indirect addressing modes: Batch 3 validates load/store effective addresses, zero-page wraparound, indirect pointer wrap, and selected page-crossing cycle behavior.

## Correctness Risks

- Flags: ALU, comparison, shift, rotate, branch, load, store, PHP/PLP, BRK, RTI, IRQ, and NMI status behavior now has focused regression coverage.
- Branch offset signed behavior: Batch 2 validates forward and backward signed relative offsets.
- Page crossing behavior: Branch taken/not-taken and page-cross cycles are validated by Batch 2. Batch 3 validates absolute indexed and `(indirect),Y` load page-cross extra cycles and fixed-cycle absolute indexed stores.
- Stack push/pop addresses: Batch 2 validates stack page `$0100-$01FF` and SP pre/post behavior for JSR/RTS and PHA/PLA. Batch 5 validates PHP/PLP/BRK/RTI status stack behavior.
- JSR/RTS correctness: Batch 2 validates return address push, RTS return target, nested subroutines, and SP restoration.
- BRK/RTI correctness: Batch 5 validates BRK vectoring through `$FFFE/$FFFF`, PC/status stack format, Break flag handling, interrupt-disable behavior, and RTI return.
- Reset vector correctness: `cpu_reset()` reads `$FFFC/$FFFD` through `mem_read16()`, which is correct for the current memory bus.
- IRQ/NMI gaps: CPU-level `cpu_request_irq()` and `cpu_request_nmi()` exist and are validated. Future device work still needs to wire peripherals to those request paths.
- Decimal mode behavior: Batch 6A validates ADC/SBC BCD arithmetic. V follows binary overflow from the unadjusted operation; C/Z/N are based on the decimal-adjusted result.
- JMP indirect page-boundary behavior: `IND()` attempts to emulate the NMOS `$xxFF` page-wrap bug. This needs a direct test.
- Cycle count availability: base cycle data exists in the lookup table and is accumulated. Branch page-cross cycles and representative indexed load/store cycles are tested; deeper full-opcode cycle accuracy remains future work.

## Monitor ROM Dependency Note

The current monitor ROM is intentionally written against a small instruction subset that already dispatches: LDX immediate, TXS, JSR, RTS, JMP absolute/indirect, LDA immediate/zero-page/absolute/absolute,X/(indirect),Y, STA zero-page/absolute/absolute,X/(indirect),Y, LDY immediate, TAX, INX, INY, CPX immediate, CPY immediate, CMP immediate, AND immediate, ORA zero-page, ASL accumulator, BEQ, BNE, BMI, and BRK-free control flow.

Batch 1 makes this subset safer by fixing Carry mutation, ORA/EOR Z flag behavior, BIT Z behavior, DEX, and NOP PC handling. Batch 3 validates the load/store addressing and bus behavior that richer monitor code will rely on. Batch 4 validates binary ALU, comparisons, shifts, and rotates. Batch 5 validates status stack behavior and the CPU-level BRK/RTI/IRQ/NMI foundation. Batch 6A validates decimal-mode ADC/SBC.

A richer monitor can now build on validated PHA/PLA/PHP/PLP, JSR/RTS, load/store addressing, comparisons, binary and decimal ADC/SBC, and BRK/RTI behavior. Breakpoints and interrupt-driven I/O still need monitor/peripheral integration on top of the CPU-level mechanisms.

## Suggested Implementation Batches

1. Batch 1: applied. Carry flag helper, ORA/EOR Z flag, BIT Z flag, DEX, and NOP PC behavior were fixed.
2. Batch 2: applied. Added focused tests for stack/subroutine/branch correctness: JSR, RTS, nested JSR, PHA, PLA, PHP, PLP, TSX, TXS, all branches, signed branch offsets, and page-cross branch cycles.
3. Batch 3: applied. Added tests for load/store addressing modes, zero-page wraparound, indexed/indirect addressing, bus behavior, ROM write protection, and UART routing.
4. Batch 4: applied. Added tests for logical ops, comparisons, binary ADC/SBC, shifts, rotates, and INC/DEC flag behavior.
5. Batch 5: applied. Added tests and hardening for BRK, RTI, PHP, PLP, IRQ/NMI entry handling, and status-bit normalization for B and bit 5.
6. Batch 6A: applied. Added NMOS decimal-mode ADC/SBC implementation and tests. Next Batch 6B work should focus on deeper cycle accuracy and any remaining profile-completion validation.
