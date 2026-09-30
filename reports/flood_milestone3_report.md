# Flood reverse engineering - Milestone 3

## Result

This pass fully reconstructs the input routine at `$13E06` and establishes the semantic shape of the main masked-sprite blitter at `$E4AA`.

## 1. Input subsystem is now understood

`$13E06` reads three physical input sources:

- CIAA serial data at `$BFEC01` for keyboard input;
- `JOY0DAT` at `$DFF00C` for joystick direction;
- an active-low CIA port bit plus `POTGO/POTINP` for two button-style hold counters.

The routine stores a raw keyboard serial byte at `$17F2A`, direction at `$17E78/$17E7A`, and button/hold counters at `$17E5A/$17E5C`.

### Direction decoding

The JOY0DAT quadrature bits are compacted into an index and used against two signed-word tables immediately before the routine:

- `$13DCE`: X direction
- `$13DEA`: Y direction

For all physically valid joystick states they produce the exact expected Cartesian values:

| Direction | dx | dy |
|---|---:|---:|
| neutral | 0 | 0 |
| right | +1 | 0 |
| left | -1 | 0 |
| up | 0 | -1 |
| down | 0 | +1 |
| up-right | +1 | -1 |
| up-left | -1 | -1 |
| down-right | +1 | +1 |
| down-left | -1 | +1 |

This is strong confirmation that `$17E78/$17E7A` are directional control values, not merely raw hardware state.

### ESC identification

The routine compares the CIA serial byte with `$75`. The Amiga keyboard serial decoding used here is rotate-right one bit followed by inversion. Applying that transformation to `$75` yields raw key `$45`, which is ESC. When ESC is seen, both hold counters are incremented.

The primary hold counter is saturated at 255.

`flood_input.c` is therefore our first clean, portable C-shaped subsystem reconstruction.

## 2. `$E4AA` is a four-plane masked sprite blitter

The routine starts with `LINK A5,#0`, takes three word arguments at `8(A5)`, `10(A5)`, and `12(A5)`, and indexes an 8-byte descriptor table at `$7D186` using the first argument.

The remaining two arguments are transformed using camera globals `$17E82` and `$17E84`, strongly indicating world/screen Y and X coordinates. The code performs explicit clipping before issuing the blit.

### Descriptor semantics

The 8-byte descriptor is accessed as:

- word `+0`: used in width/modulo/BLTSIZE arithmetic;
- word `+2`: used as the other sprite dimension and in plane-size multiplication;
- long `+4`: added into source address registers, therefore a sprite-data pointer or offset.

### Blitter operation

The routine waits for DMACONR bit 6 before each launch and programs:

- `BLTCON0/BLTCON1`
- first/last word masks
- A/B/C/D modulos
- A/B/C/D pointers
- `BLTSIZE`

`BLTCON0` is constructed from horizontal shift bits plus `$0FCA`. Minterm `$CA` is the classic cookie-cut masked combination in this A/B/C/D arrangement: source mask/image are combined with the existing background and written back to destination.

The pointer roles visible in the code are consistent with:

- A: mask source
- B: image source
- C: existing destination/background
- D: destination

### Four bitplanes

The BLTSIZE launch sequence is repeated four times. Between launches:

- the destination pointer advances by `$2940` bytes;
- sprite source pointers advance by the calculated per-plane source size.

This demonstrates that Flood's main drawing path here is operating on a four-bitplane display/object representation.

The destination row modulo calculation uses constant `44`, so the working screen/buffer stride is 44 bytes per raster line (352 pixels of planar storage, including any margins/padding used by the engine).

## 3. Emerging C interfaces

The reconstructed architecture now supports concrete portable interfaces such as:

```c
void flood_poll_input(FloodInputState *state, ...);
int flood_blit_masked_sprite(uint16_t sprite, int16_t world_y, int16_t world_x);
```

The input function is already reconstructed closely enough to implement. The blitter interface is semantically secure, while its clipping arithmetic still needs a branch-by-branch lift before it should be called exact C.

## 4. Updated symbols

| Address | Reconstructed meaning | Confidence |
|---|---|---|
| `$13DCE` | joystick X lookup | very high |
| `$13DEA` | joystick Y lookup | very high |
| `$13E06` | `PollInput` | very high |
| `$17E78` | input dx | very high |
| `$17E7A` | input dy | very high |
| `$17E5A` | primary/escape hold counter | high |
| `$17E5C` | secondary/escape hold counter | high |
| `$17F2A` | raw CIA keyboard SDR byte | very high |
| `$E4AA` | clipped four-plane masked sprite blit | very high |
| `$7D186` | 8-byte sprite descriptor table | high |
| `$17E82` | camera/viewport Y | high |
| `$17E84` | camera/viewport X | high |
| `$7C6F2` | screen/display buffer pointer used by blitter | high |

## Next milestone

1. Lift every branch of `$E4AA` into exact C, including left/right/top/bottom clipping.
2. Inspect several `$E4AA` call sites to verify argument order and identify the sprite descriptor table contents.
3. Decode the `$170xx` audio routine to recover the sample/channel structure.
4. Begin a real source tree (`src/input.c`, `src/blitter.c`, `include/flood.h`) rather than standalone pseudocode.
