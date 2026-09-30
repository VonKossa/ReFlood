# Flood reconstruction — Milestone 102

## Original ending viewport and aspect ratio

The ending was being presented as 320x240 because each `END_SCRE` bitplane has
240 stored rows. That is the backing allocation, not the original visible
display window.

The Copper list built by `$EE4A` writes these display registers:

| Register | Value | Result |
| --- | --- | --- |
| `DIWSTRT` | `$2C81` | first visible ending scanline |
| `DIWSTOP` | `$FCC1` | 208-line vertical display boundary |
| `DDFSTRT` | `$0030` | original horizontal fetch start |
| `DDFSTOP` | `$00D0` | original horizontal fetch stop |

The correct logical viewport is therefore 320x208. The existing Copper pointer
offset and fetch delay already select source columns 0–319, so no horizontal
crop change is needed.

## Cause of the artefact

`END_SCRE` deliberately retains a 352x240 planar backing with a 44-byte row
stride. The original display clips rows 208–239. The first pixel in that hidden
region appears at `(133,208)` with palette index 12, directly matching the
unwanted material visible below the shaft when the host exposed all 240 rows.

Milestone 102 keeps the full backing for all 75 original patch blits but limits
pixel presentation, the SDL texture, window size, logical size, and fade to
320x208. This removes the lower artefact and restores the ending's wider
original proportions.

## Regression coverage

- the visible ending dimensions are fixed at 320x208;
- the backing remains 352x240 with `$2940` bytes per plane;
- the known color-12 pixel at backing `(133,208)` remains stored but is clipped
  by the public ending pixel path;
- base, intermediate, and final planar hashes remain byte-exact;
- the complete 334-frame/75-event animation timing remains unchanged;
- the SDL ending stage uses the 320x208 logical dimensions.

## Verification

- C99 `-Wall -Wextra -Wpedantic -Werror` core tests: pass
- C99 `-Wall -Wextra -Wpedantic -Werror` SDL input-stub tests: pass
- UndefinedBehaviorSanitizer core tests: pass
- UndefinedBehaviorSanitizer SDL input-stub tests: pass
- Headless load/run smoke test for all 42 levels: pass
