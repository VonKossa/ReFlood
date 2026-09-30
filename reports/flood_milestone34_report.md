# Flood reverse engineering — Milestone 34

## First SDL-ready real-cavern integration

This milestone replaces the synthetic/debug terrain used by Milestone 33 with the original decoded Flood map and tile assets.

### Real level loading

`flood_game_load_level()` loads any level from 1 through 42. It reads the 16-byte level control block, takes the first big-endian word as the graphics-bank selector, loads the corresponding BLOCKA/B/C terrain bank and attribute table, and converts the Amiga four-bitplane 16x16 tiles into host-side 4-bit chunky pixels.

The original terrain remains 128 x 100 tiles (2048 x 1600 pixels).

### Real Quiffy start

Marker 22 is consumed as the player spawn point. For level 1 this places Quiffy at tile `(6,5)`, i.e. pixel `(96,80)`. Setup/spawn markers 1..25 are removed from the visual terrain after their positions are inspected; future milestones will instantiate their corresponding runtime objects instead of merely clearing them.

### SDL renderer architecture

When SDL2 is available, `flood_host` creates a 352 x 240 logical viewport shown at 2x scale. The camera follows Quiffy and clamps to the cavern bounds. Terrain is rendered from the original tile indices and original tile pixel data.

The palette is intentionally provisional. The important invariant for this milestone is that the tile art and palette indices are original; exact Amiga palette/copper recovery can be integrated later without changing the tile/map API.

### Headless portability

SDL2 is not installed in the current container, so the SDL branch cannot be executed here. The same source builds in headless mode, allowing the loader/core to be tested independently of the platform layer.

Verified here:

- clean CMake build;
- `ctest` passes;
- level 1 loads with bank A and start marker `(96,80)`;
- level 23 loads through the same generic path;
- planar tile conversion produces non-empty pixel data;
- collision reads the real selected BLOCK attribute table.

### Next integration step

The highest-value next step is to replace the player outline with the real Quiffy sprite pipeline and integrate the exact `$F0E0/$F50A` collision routines. After that the SDL host will have original cavern art, original player art, and substantially original movement/collision semantics in the same executable.
