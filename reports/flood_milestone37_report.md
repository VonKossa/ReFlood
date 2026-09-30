# Flood reverse engineering - Milestone 37: exact gameplay palette

## Result

The SDL renderer now uses the original gameplay palette rather than a provisional host palette.

The Copper-list builder at `$B64E` emits `COLOR00..COLOR31`. It takes its 32 source words from the table at `$B60E`. The first 16 entries are the palette used by the four-bitplane cavern and software-blitted sprites.

Recovered `COLOR00..COLOR15` values:

`000 FD9 D84 B43 823 613 403 202 024 146 268 38A 120 230 340 450`

These are Amiga 12-bit `0xRGB` values. SDL conversion expands each 4-bit channel to 8 bits by multiplying it by 17.

The full 32-register table is documented in `docs/gameplay_palette.md`.

## Verification

- SDL host source updated to the recovered palette.
- Headless build succeeds.
- CTest passes.
- Level 1 preview regenerated from original tile/sprite indices using the recovered palette.

## Note

The palette table is fixed in the gameplay Copper setup we traced. Other screens (title/instructions/end screens) use separate display setup paths and may have different palettes; this milestone concerns the in-game cavern renderer.
