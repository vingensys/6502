# Batch 1 CPU Smoke Tests

There is no automated CPU test harness in this repository yet. These byte
sequences are focused debugger smoke tests for the Batch 1 opcode fixes.

Each sequence is intended to be loaded at `$8000`:

```sh
printf '<bytes>' > batch1.bin
./bin/emulator.out batch1.bin
```

Then step in DEBUGGER mode and inspect PC/register/flag state.

| Behavior | Bytes | Expected result |
|---|---|---|
| SEC sets Carry | `\x38` | C flag becomes `1`. |
| CLC clears Carry | `\x38\x18` | After SEC then CLC, C flag becomes `0`. |
| ORA zero result sets Z | `\xA9\x00\x09\x00` | A remains `$00`, Z becomes `1`, C is unchanged by ORA. |
| EOR zero result sets Z | `\xA9\x55\x49\x55` | A becomes `$00`, Z becomes `1`, C is unchanged by EOR. |
| BIT checks full `A & M` | `\xA9\xF0\x85\x10\xA9\xF0\x24\x10` | BIT sees `$F0 & $F0 != 0`, so Z is `0`; N is `1`; V is `1`. |
| DEX wraps 0 to FF | `\xA2\x00\xCA` | X becomes `$FF`, N becomes `1`, Z becomes `0`. |
| NOP consumes only opcode | `\xEA\xEA` | After one step PC advances from `$8000` to `$8001`, not `$8002`. |
