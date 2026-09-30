# Flood reverse engineering — Milestone 56

## Result

Milestone 56 completes the main 128-record object dispatcher at
`$CC60-$CDA0`, including behavior outside the individual handler calls:

- signed y-bound state clearing;
- the exact 25-entry jump table;
- five proven no-op states;
- the out-of-range-state skip;
- the deferred State-24-to-State-12 bridge.

All 15 unique nonempty handler targets were already implemented by Milestone
55; this pass supplies the missing dispatcher shell around them.

## Per-record prologue

For each 20-byte record, the original loads `y` from `+2` and compares it with
`$0640` (1600). A signed `y >= 1600` writes State 0 at record `+12` before the
jump-table lookup. Negative y values remain eligible for dispatch.

The host now performs the same active-record cull before grouped object
updates. The State-1/10 slot-order loop repeats the guard because State 1 can
activate the following companion slot after the global prepass has visited it.

State values above 24 branch directly to the record-loop tail without changing
the record.

## Complete jump table

| State | Target or action | Host status |
| ---: | --- | --- |
| 0 | no-op (`$CCB8`) | exact |
| 1 | `$13782` | exact |
| 2 | `$1091A` | exact |
| 3 | no-op (`$CCD8`) | exact |
| 4 | no-op (`$CCDC`) | exact |
| 5 | `$13972` | exact |
| 6 | `$10A10` | exact |
| 7 | no-op (`$CCF4`) | exact |
| 8 | `$10834` | exact |
| 9 | `$10A10` | exact |
| 10 | `$1385A` | exact |
| 11 | `$1306A` | exact |
| 12 | `$10A92` | exact |
| 13 | `$1091A` | exact |
| 14 | `$10C46` | exact |
| 15 | `$10A10` | exact |
| 16 | `$133E6` | exact |
| 17 | `$12EA4` | exact |
| 18 | `$12FBE` | exact |
| 19 | `$FD28` | exact |
| 20 | `$1091A` | exact |
| 21 | `$FB42` | exact |
| 22 | `$1363A` | exact |
| 23 | no-op (`$CD92`) | exact |
| 24 | write State 12 (`$CD94`) | exact deferred bridge |

State 0 contains a comparison of object-loop index `d7` with `$77`, but both
the taken branch and fall-through branch go to `$CD9A`. It has no observable
effect and is therefore a proven no-op, not an unresolved branch.

## Marker values are not runtime states

The same numeric value can have a distinct map-initializer role:

- marker 3 (34 static, 1 active payload) clears the packed tile-3 graphics and
  does not allocate State 3;
- marker 7 (4 static) clears its map cell and does not allocate State 7;
- marker 23 (12 static) configures level bounds and does not allocate State 23;
- marker 24 (112 static) directly allocates State 12 Vong objects.

Markers 4 and 24 likewise do not produce runtime States 4 or 24. Runtime State
24 has no shipped initializer or resident-image writer and is a dormant safety
bridge. State 0 is live as the inactive/completed state and as the six reserved
companion records allocated beside the six static Doctor Dusty objects.

## Tests

Focused tests verify:

- States 0, 3, 4, 7, and 23 preserve every object field;
- an out-of-range State 25 is skipped unchanged;
- State 24 changes only to State 12 on its first tick;
- State-12 motion/drawing begins on the following tick;
- `y=1600` clears State 5 before its handler can move it;
- `y=1599` remains dispatchable;
- no shipped level load creates runtime States 3, 4, 7, 23, or 24.

## Next boundary

The main runtime object dispatcher is now structurally complete. The next pass
should return to the unresolved Sparkling Fungi/static-hazard question by
tracing the map-attribute and direct tile-contact paths outside this object
jump table.
