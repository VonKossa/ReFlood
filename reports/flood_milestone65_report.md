# Flood Milestone 65: exact secondary overlay and HUD vertical correction

## Result

Milestone 65 reconstructs the second tile compositor at `$ED66-$EE48` and
corrects the status strip to the uppermost visible scanline. The SDL output
now draws the HUD at visible `(0,0)` using its one-bit mask only; no opaque HUD
rectangle is introduced.

## Correct W_BLOCKS recovery

Runtime pointer `$17F22` is set to `$10ED0`, but that address is a destination
overwritten during setup. Its initial resident bytes belong to copy-protection
text and are not overlay art. The custom disk directory instead names the
source explicitly as `W_BLOCKS` on track 86.

`W_BLOCKS` is a 1,150-byte packed stream whose header declares an unpacked
size of `$1500` (5,376). The existing Flood back-reference format expands it
to two adjacent `$0A80` banks. Each bank contains 42 records of 64 bytes:

| Record portion | Meaning |
| --- | --- |
| bytes 0-31 | 16x16 source for display colour bit 2 |
| bytes 32-63 | 16x16 source for display colour bit 3 |

The extracted `W_BLOCKS.bin` SHA-256 is
`fc26f6c095b1588bb00ea5e2b4558feacfff85127583b75804ef4cc4817ae8a9`.
The packed disk member SHA-256 is
`64903dcbc063e3cbcb48992a4ae5239696e887855a8960bb1f573c80bfbc0a01`.

## Exact compositor

The caller supplies `$17F16 + coarse_camera_index` as a secondary 128x100 map,
`$17F22 + $17E86*$0A80` as the graphics bank, and the same fine-camera
destination phase used by terrain. The routine uses `DBRA` counters 13 and 21,
therefore traversing exactly 14 rows by 22 columns.

A zero secondary-map byte skips the cell. A nonzero byte is multiplied by 64
to choose its record. The two 16-line, one-word blits target display planes 2
and 3. `BLTCON0=$0DFC` selects A OR B into D, so the equivalent chunky rule is:

```text
display_colour = terrain_colour | (overlay_2bit << 2)
```

The host implements this as `flood_composite_overlay_window()` after the
normal terrain pass. It preserves zero-state transparency, clips the exact
overfetch window, and selects the bank with the alternating display-buffer
index. This milestone does not invent the still-unreconstructed propagation
routine; it renders the secondary states that the reconstructed game produces.

## Why the HUD is visible at Y=0

The `$14554` call passes Y=16, which first adds `16*44=$2C0` bytes. However,
the destination pointer `$17E54` is already `frame-$2C0` for camera phases
1-15 and `frame-$2BE` for phase zero. The sum therefore lands at frame byte 0
or 2, not 16 rows below the frame. Just as the horizontal fetch word must be
separated from visible X, this Y argument is a backing-buffer coordinate.

The visible strip consequently spans Y=0-7. `$14598-$145E4` clears and sets
only bits selected by its one-bit mask, leaving all other terrain pixels
untouched. The SDL compositor now matches both properties.

## Verification

- strict C99 `-Wall -Wextra -Wpedantic -Werror`: pass;
- complete core regression: `core ok`;
- UBSan with abort on first error: `core ok`;
- all 42 headless levels complete their 300-tick run;
- overlay tests cover disabled and zero states, both banks, high-bit OR
  semantics, fine-phase clipping, and guarded invalid state input;
- HUD tests assert backing Y=16 and visible Y=0 separately.
