# Flood Milestone 52 — dormant State 11

State 11 is a complete but unused horizontal counterpart to State 18. The
dispatcher calls `$1306A` unconditionally; unlike States 16–18, State 11 is not
gated by `$E1EA`.

## Shipped-level reachability correction

All 42 decoded static maps contain zero marker-11 bytes. A second audit parsed
each active 10-byte trigger record, followed its payload offset and rectangle
size, and inspected only bytes that can actually be swapped into the map. The
active payload regions also contain zero marker-11 cells.

Several complete 640-byte level chunks contain the numeric byte value 11 in
unused or structural positions, but none is a reachable marker placement. The
handler and initializer exist in the executable, yet the shipped levels do not
instantiate them. Milestone 52 preserves that distinction.

## Initializer (`$E8EA-$E92A`)

Marker 11 creates State 11, saves `(x,y)` as the second endpoint pair, clears
animation and word `+$0A`, sets phase byte `+8` to 3, and replaces the marker
with `$CB`. The host initializer now explicitly maps word `+$0A` to timer zero.

## Activation (`$130A2-$13104`)

Phase 3 requires the terrain cell immediately left of `(x,y)` to be empty. It
then tests a 32×24 rectangle at `(x-8,y-8)` against Quiffy's centered body. An
overlap leaves the mechanism dormant. Otherwise the phase changes directly
from 3 to 1. No sound routine is called on this transition.

## Horizontal growth (`$13230-$132CC`)

Phase 1 extends the saved/high endpoint right by 16 pixels and the low endpoint
left by 16 pixels. Before each end advances, the routine checks the flood-state
grid at `$17F16`. A nonzero destination changes the object to State 0 and
returns immediately. Empty terrain destinations receive `$CC`: the shared
horizontal placement helper starts with `$C8` and adds four specifically when
the object state is 11.

The wrapper saves and restores Quiffy's Life Force and the original overlap
latch when State 11 remains active. If the second flood check ends the object
after the first endpoint was processed, the original routine intentionally
skips that restoration; the host preserves the same ordering for Life Force.
Any `$CC` tiles already written remain in the map after State 0.

## Validation

Tests cover exact marker initialization, phase-3 terrain and player blocking,
the silent 3→1 transition, two-ended `$CC` growth, endpoint coordinates,
preserved player life, flood-state termination, retained tiles, and State 11's
independence from the `$E1EA` pause gate. Strict C99, UBSan, and all-level smoke
tests also pass.
