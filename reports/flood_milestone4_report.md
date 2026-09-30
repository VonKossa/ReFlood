# Flood reverse engineering - Milestone 4

## Result

This pass completes a branch-by-branch lift of the central blitter at `$E4AA` and corrects the camera-axis labels using direct caller evidence.

## 1. Correct function signature

`$E4AA` is called using the 68000 stack convention and reads:

```c
int BlitSprite(uint16_t sprite, int16_t world_x, int16_t world_y);
```

Call sites around `$AFDC` are decisive: they push `cameraY + 100` first (third argument), then `cameraX + localX` (second argument), then sprite `$4A` (first argument).

Therefore:

- `$17E82` = camera X
- `$17E84` = camera Y

The Milestone 3 report had these two labels reversed.

## 2. Descriptor format

The sprite index selects an 8-byte entry at `$7D186`:

```c
struct SpriteDesc {
    uint16_t row_bytes;   // +0
    uint16_t height;      // +2
    uint32_t source;      // +4
};
```

The interpretation is now stronger than before because `$E4AA` computes:

```text
plane_size = row_bytes * height
```

and advances the image pointer by exactly `plane_size` after each destination bitplane.

## 3. Source layout

The source pointer describes five packed blocks:

```text
source + 0 * plane_size   image bitplane 0
source + 1 * plane_size   image bitplane 1
source + 2 * plane_size   image bitplane 2
source + 3 * plane_size   image bitplane 3
source + 4 * plane_size   common 1-bit mask
```

The mask pointer is not advanced between the four launches, while the image pointer advances by `plane_size`. This establishes the layout directly from the machine code.

## 4. Clipping behavior

The routine has asymmetric clipping by design.

### X

It first requires `world_x >= 0`, then computes:

```c
guarded_x = world_x - camera_x + 32;
```

and rejects when `guarded_x <= 0` or `guarded_x > 352`.

There is no per-pixel left/right sprite clipping. The renderer instead works in a 352-pixel-wide, 44-byte-row buffer with a 32-pixel guard region. This explains why the earlier pass appeared to show unusual horizontal clipping.

### Y

Y is genuinely clipped against a 240-row viewport:

- if the sprite begins above the viewport, source rows are skipped and visible height is reduced;
- if it extends below row 239, only height is reduced;
- wholly invisible sprites return `-1`.

Top clipping changes the source start by `(-screen_y) * row_bytes`. Bottom clipping leaves the source start unchanged.

## 5. Exact planar/mask behavior

The original uses:

- 4 destination planes;
- destination plane stride `$2940`;
- 44 bytes per destination row;
- one common mask after four image planes;
- BLT minterm `$CA`;
- A = mask;
- B = current image plane;
- C = existing destination;
- D = destination.

This is a textbook masked cookie-cut operation, repeated once per color bitplane.

The blit width is `(row_bytes + 2) / 2` words. The extra two bytes provide the extra word needed by shifted planar blitting.

## 6. Horizontal alignment

The shift is exactly:

```c
shift = (world_x & 15) << 12;
BLTCON1 = shift;
BLTCON0 = shift | 0x0FCA;
```

The destination address uses rotate/subtract arithmetic on `world_x` and `camera_x`, effectively deriving the word-column delta while preserving the original 68000 behavior. When `camera_x` is 16-pixel aligned, the code applies an additional two-byte destination correction.

## 7. Return value

- `0` = blit issued successfully
- `-1` = rejected before touching the blitter (off-screen/invalid coordinate)

## 8. New portable implementation

`src/blitter.c` reconstructs the arithmetic as a `FloodBlitPlan` rather than writing directly to Amiga hardware. This makes it testable on a modern host while retaining the original pointer, clipping, shift, modulo and plane calculations.

The C source compiles cleanly with warnings enabled.

## Next milestone

The renderer is now sufficiently understood to move up one abstraction level. The most useful next target is to identify the object/entity records at the dense `$136xx-$13Bxx` call sites, because those routines repeatedly load X/Y/state fields and invoke `$E4AA`. Recovering that structure should give us the first real `FloodObject`/`Entity` C type and connect rendering to gameplay state.
