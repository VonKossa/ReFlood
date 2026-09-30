# Flood reverse engineering - Milestone 6

## Main result: `$F0E0` is the tile-collision rectangle query

The shared routine at `$F0E0` has now been reconstructed with high confidence.

Its six word arguments are:

```c
F0E0(x, y, dx, dy, width, height)
```

The routine works on a **16x16-pixel tile grid**. It converts pixel coordinates to tile coordinates with `LSR.W #4` and converts tile Y to a byte offset using a **128-byte map row stride** (`LSL.W #7`).

Two pointers are read from globals:

- `$17F0A`: tile map containing tile IDs.
- `$17F1A`: tile-attribute table indexed by tile ID.

This two-stage lookup is visible directly in the inner loops: the routine reads a tile byte from the map and then uses that byte as the index into the attribute table.

## Three outputs

The function stores three OR-combined collision results:

- `$17E60`: flags on the vertical edge reached by **X movement**.
- `$17E62`: flags on the horizontal edge reached by **Y movement**.
- `$17E64`: flags at the moved rectangle's **diagonal corner**.

When moving horizontally, the routine scans every tile touched along the appropriate left/right edge. When moving vertically, it scans every tile along the top/bottom edge. When movement exists on both axes it also probes the diagonal corner separately.

## Why the separate corner result exists

A rectangle moving diagonally can have clear X and Y edge scans while its newly-entered corner overlaps a special tile. Flood therefore keeps the corner attribute separate instead of simply merging all three tests.

## Collision attribute bit 1

Many independent callers execute `BTST #1` on `$17E60` or `$17E62` and then cancel, reverse, or otherwise resolve movement. Bit 1 can therefore be named a **solid/blocking collision class** with high confidence.

Other bits are used by callers, but they have deliberately not been assigned semantic names yet.

## Object-layout payoff

The routine at `$13972` loads:

```text
+00 -> x
+02 -> y
+04 -> D3
+06 -> D4
```

then calls:

```c
F0E0(x, y, D3, D4, 32, 32);
```

and finally performs `x += D3`, `y += D4`, writing all four values back. For this handler family, `+04/+06` are definitively **dx/dy motion components**.

This also validates Milestone 5's conclusion that the same slots are type-specific: another handler uses them as saved coordinates, so Flood's object record is a common header with handler-specific payload semantics.

## Reconstructed portable implementation

`src/collision_f0e0.c` contains a host-portable implementation preserving the key 68000 behaviors:

- 16-bit coordinate arithmetic,
- unsigned `/16` tile conversion,
- 128-byte tile rows,
- edge choice based on movement sign,
- tile-ID -> tile-attribute lookup,
- separate X, Y, and corner results.

It compiles cleanly with strict warnings.

## Next target

The collision primitive is now understood well enough to move up one level. The best next milestone is to reconstruct one complete moving-object handler (starting with `$13972`) including its collision response, rendering, and object-interaction call at `$10DCA`. That should reveal what kind of object it is and begin turning the anonymous dispatcher into named gameplay entities.
