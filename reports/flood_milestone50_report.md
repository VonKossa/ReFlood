# Flood Milestone 50 — State 8 Vacuous Gombo

The object dispatcher selects `$10834` for runtime State 8. Marker 8 enters
through `$E8C6`, which records the map position and state, clears the marker,
and otherwise leaves velocity and animation zero. There are 80 static marker-8
placements across the 42 original levels.

## Collision and movement (`$10834-$10918`)

The handler queries `$F0E0` with a 24×32 collision body at `(x+4,y)` and the
stored velocity. Solid X and Y edge results zero only the corresponding local
motion component. If either component survives, position advances using those
locals while the stored velocity remains unchanged. A wall can therefore stop
horizontal movement for one update while the object continues vertically and
tries its original horizontal component again next update.

When both local components are zero, `$12990` selects and stores a new heading
at speed 4, but the current update still commits zero motion. Animation and
contact continue during this deliberate one-update pause.

## Eight-direction selector (`$12990-$12A36`)

The first RNG result is reduced to one bit and stored at record word `+$0A`.
If zero, the heading is selected from the relative signs of Gombo's position
and Quiffy's position. If one, a second RNG result selects direction 0–7.

| Index | Velocity | Index | Velocity |
| ---: | ---: | ---: | ---: |
| 0 | `(0,-4)` | 4 | `(0,4)` |
| 1 | `(4,-4)` | 5 | `(-4,4)` |
| 2 | `(4,0)` | 6 | `(-4,0)` |
| 3 | `(4,4)` | 7 | `(-4,-4)` |

The equal-position chase-table entry is direction 0 rather than a zero vector.

## Animation and contact

The alternating `$17E86` step wraps through original frames `$90-$93`.
Unlike the pre-move State-12 walkers, Gombo draws at its committed position.
`$10DCA` tests the same 24×32 body at `(x+4,y)` against Quiffy's centered
16×16 body and removes 8 Life Force on overlap.

Tests cover chase and random selection with exact RNG advancement, all eight
direction semantics, the equal-position entry, one-axis cancellation with
retained stored velocity, two-axis reselection and pause, animation timing,
render position, contact damage, marker population, strict C99, and UBSan.
