# Flood Milestone 51 — terrain mechanisms, States 16–18

The dispatcher gates States 16, 17, and 18 on global word `$E1EA`; when it is
nonzero none of the three handlers runs. The host exposes the same gate as
`world.mechanisms_paused`.

## Initializers and runtime representation

Markers 16 and 17 save their initial `(x,y)` into record words `+4,+6`, choose
an unsigned RNG remainder from 1–100 for word `+$0A`, start in phase 0, and
replace the marker with `$C9` or `$CD`. Marker 18 saves the same endpoints,
starts in phase 3 with timer zero, and becomes `$CB`.

The host uses `(x,y)` for the low endpoint and `(origin_x,origin_y)` for the
saved/high endpoint. These correspond directly to the two coordinate pairs in
the original 20-byte record.

## State 16 — horizontal cycle (`$133E6-$1360C`)

Phase 0 counts down, enters phase 3, and requests sound 49 if the mechanism is
inside the recovered camera range. Phase 3 tries to extend both endpoints by
16 pixels, writing `$C8` into empty cells. A new 16×16 cell overlapping Quiffy
removes 16 Life Force and forces the transition early. When both ends are
blocked, the mechanism enters a 50-update phase-2 wait. At expiry it requests
sound 50 and enters phase 1. Phase 1 removes `$C8` cells inward until both ends
reach the `$C9` center, then returns to phase 0 with another 50-update delay.

## State 17 — vertical cycle (`$12EA4-$133E4`)

State 17 has the same four-phase timer and sound sequence, oriented vertically.
It writes and removes `$CA` around its fixed `$CD` center. Placement collision,
blocked-end counting, player damage, and the forced early contraction path are
instruction-equivalent to State 16's horizontal operations.

When mechanism triggering is enabled, both placement helpers recognize `$FF`
cells and call the mutable trigger routine with that map index. Nonempty cells
still block endpoint advancement.

## State 18 — flood-limited growth (`$12FBE-$1322E`)

Phase 3 waits until the cell immediately above the `$CB` center is empty and a
32×24 activation rectangle at `(x-8,y-8)` does not overlap Quiffy. It then
enters phase 1 and requests sound 26.

Phase 1 grows vertically at both ends, writing `$CC`. Before each endpoint it
checks the secondary 128×100 grid addressed through `$17F16`. The flood engine
initializes, scans, and updates this same grid, confirming it as the flood-state
map. A nonzero destination ends the State-18 object immediately. Player-life
and overlap side effects from the shared placement helper are restored while
State 18 remains active, matching the wrapper at `$12FBE`.

## Validation

Tests cover both complete cyclic mechanisms, exact tile IDs, endpoint motion,
50-update waits, sound events 49/50, player obstruction and 16-point damage,
the `$E1EA` pause gate, State-18 activation blocking, sound 26, `$CC` growth,
preserved player life, flood-state termination, and all 159 original marker
placements.
