# Flood reverse engineering — Milestone 24

## Summary

This milestone fully recovers the custom compression used by `SPR_16.B`, renders all 80 16x16 sprites, corrects the global sprite-table ordering, and visually confirms several creature mappings.

## `SPR_16.B` packing format

The file begins with two big-endian longwords:

- packed size: `$176C` = 5996 bytes (exact file size)
- unpacked size: `$3200` = 12800 bytes

12800 bytes is exactly `80 * $A0`, matching the 80 descriptors built by `$B3EC`.

The unpacker is `$1499C`. Its stream is a simple signed-word LZ/RLE hybrid:

- read signed 16-bit control
- `control >= 0`: copy `control + 1` literal bytes from input
- `control < 0`: read unsigned 16-bit absolute offset into already-produced output and copy `-control + 1` bytes forward
- overlapping back-references are legal

The loader compares actual file size with expected unpacked size. If equal, no unpacking occurs (`SPR_32.B`). If different, the packed bytes are moved to the end of the allocated output buffer and `$1499C` expands them in-place-safe fashion.

## Corrected global sprite ordering

A previous milestone had the descriptor order reversed. The actual sequence is:

- global `$00-$4F`: `SPR_16.B` (80 sprites)
- global `$50-$E5`: `SPR_32.B` (150 sprites)

The source layout for both banks is mask-first, then four image planes.

## Cast confirmations

### Snail

State-12 subtype `$66` is now visually confirmed. The shared renderer adds base `$50`, producing frames beginning at global `$B6`; `$B6-$BD` visibly show a crawling snail with a large spiral shell.

### Plonkin Donkin

State 14 uses global `$DE-$E1`. Those frames show a sleepy/bleary-eyed round creature, strongly matching Plonkin Donkin's documented leaping behavior. This is now visual plus behavioral confirmation.

### Psycho Teddy / Vong

Subtype `$38` maps to global `$88-$8F`; subtype `$6E` maps to `$BE-$C5`. Their visual groups are coherent with their already-decoded behavior, strengthening both mappings.

### Vacuous Gombo correction

The runtime ID `$90-$93` remains correct, but those IDs map to SPR_32 entries 64-67 after the corrected global-table ordering. The earlier statement that raw SPR_32 entries `$90-$93` were used was incorrect.

### State 22

The previous Sparkling Fungi guess is downgraded to unresolved. Its base `$14` sits in the 16x16 bank, but the recovered composite sprites do not independently establish a fungus identity.

## Files

- `src/flood_unpack.c`: clean C reconstruction of `$1499C`
- `include/flood_unpack.h`
- `evidence/spr16_global_00_4f_contact_sheet.png`
- `evidence/cast_sprite_groups.png`
- `sprite_descriptor_order.md`
- `cast_visual_confirmations.md`

## Next target

Use the corrected global sprite map to trace state 22's spawn context and the remaining anonymous runtime states, while also following the indirect creator of the three-hit `23 -> 24 -> 25 -> 26` destructible. The corrected descriptor map also makes a complete runtime object-to-art atlas practical.
