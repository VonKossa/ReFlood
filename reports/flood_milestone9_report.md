# Flood reverse engineering — Milestone 9

## Target

Decode RUN_PROG routine `$F50A`, determine its exact inputs/outputs from callers, and fold the result back into the reconstructed Psycho Teddy movement code.

## Result

`$F50A` is a **single-point tile attribute lookup**.

High-level signature:

```c
uint8_t flood_tile_attributes_at(const FloodTileMap *map,
                                 int16_t x,
                                 int16_t y);
```

It converts pixel coordinates to a 16x16 tile coordinate, indexes the 128-tile-wide map, reads the tile ID, then uses that tile ID to fetch an attribute byte.

Runtime globals used by the original routine:

- `$17F0A` -> tile-ID map
- `$17F1A` -> tile attribute table

## Exact map-index calculation

The original 68000 sequence initially looks unusual:

```asm
move.w  y,d0
lsr.w   #4,d0
swap    d0
move.w  x,d0
lsl.w   #5,d0
lsl.l   #7,d0
swap    d0
```

For ordinary in-map coordinates, this is exactly equivalent to:

```c
index = ((y >> 4) << 7) + (x >> 4);
```

or:

```c
index = (y / 16) * 128 + (x / 16);
```

This independently confirms two earlier findings:

- tile dimensions are 16x16 pixels;
- map rows contain 128 tile entries.

A host-side equivalence test reproducing the register operations was run over a large coordinate range and passes against the direct C formula.

## Call-site evidence

### Psycho Teddy at `$13B5C`

The handler passes:

```text
x + 16
y + 28
```

and immediately tests bit 1 of the returned attribute byte.

The exact logic is now:

```c
if ((flood_tile_attributes_at(map, x + 16, y + 28) & 2) == 0) {
    x += dx;
    y += dy;
    if (dx != 0)
        object->dx = dx;
}
object->dy = dy;
```

So the routine is not a broad support/collision solver. It is a cheap point-sample used after the larger swept collision query.

### Caller at `$10CB6`

A second handler passes approximately:

```text
x + 8
y + 8
```

and again tests attribute bit 1. This independently confirms that `$F50A` is generic point terrain lookup rather than Teddy-specific logic.

## Relationship with `$F0E0`

The two routines now form a clear pair:

```text
$F0E0  -> swept rectangular tile collision query
$F50A  -> single pixel-position tile attribute lookup
```

Both eventually use the same tile attribute system, and callers consistently treat attribute bit 1 (`0x02`) as a blocking/solid terrain class.

## Psycho Teddy refinement

The previous provisional helper:

```c
flood_probe_support(...)
```

has been removed. The updated reconstruction uses the exact tile-map API:

```c
flood_tile_attributes_at(map, x + 16, y + 28)
```

This reduces another unknown engine primitive to a portable and directly testable function.

## Files

- `src/tile_query_f50a.c` — exact high-level lift of `$F50A`
- `include/flood_tilemap.h` — tile-map API
- `tests/test_f50a_index.c` — verifies the shift/SWAP sequence against the direct index formula
- `f50a_semantic_disassembly.s` — annotated original routine
- `src/psycho_teddy_13a4c_updated.c` — Psycho Teddy with `$F50A` semantics integrated
- `src/psycho_teddy_13ba8.c` — previously recovered tile-eating helper

## Next target

The best next target is the group around `$F630/$F698` immediately following `$F50A` in RUN_PROG. Those routines are repeatedly used by higher-level terrain/object code and appear to build on the same map representation. Recovering them should expose more of the map mutation/redraw pipeline and reduce the remaining opaque calls in the enemy handlers.
