# Flood Milestone 41 — corner poses and proved object links

## `$D460-$D4F0`

This routine is now translated exactly. It runs when the eight-direction ring
has exactly one contact. Pose `$55` is the sentinel; recognized diagonal and
orientation combinations select `$56-$5D`. The special renderer adds `$50`,
so the visible sprites are `$A6-$AD`. These are separate from ordinary walking
sprites `$C8-$CF`.

| Contact | Orientation condition | Pose | Sprite |
| --- | --- | ---: | ---: |
| SE | right and down | `$56` | `$A6` |
| NE | left and down | `$57` | `$A7` |
| NW | left and up | `$58` | `$A8` |
| SW | right and up | `$59` | `$A9` |
| SE | facing left | `$5A` | `$AA` |
| NE | moving up | `$5B` | `$AB` |
| NW | facing right | `$5C` | `$AC` |
| SW | moving down | `$5D` | `$AD` |

All other single-corner orientation combinations retain sentinel `$55` and do
not enter the special pose renderer.

## Objects connected

- Marker 19 now creates runtime state 19 and replaces itself with Mine tile
  `$E6`, matching the initializer at `$EA8C`.
- State 19 uses the proved `(x+4,y+12)`, 8x4 contact rectangle. It clears its
  map tile, becomes state 20, resets animation, reanchors by (-8,-16), and
  primes the three-tick upward impulse while Quiffy is alive.
- State 20 implements the four explosion phases at `$1091A`; phase zero deals
  40 Life Force damage on its 32x32 overlap, then it becomes state 21 Heart.
- Heart collection restores 64 Life Force capped at 511 and awards 10 points.
- `$E7` Parachute and `$E8` Balloon now use the cached player-center map-cell
  dispatch at `$FC76-$FD26`. The tiles remain present; entering a new cell is
  what permits another pickup dispatch.

## Validation

Strict C99 compilation with `-Wall -Wextra -Wpedantic -Werror` passes. The test
suite covers all sixteen combinations of four diagonal contacts and four
horizontal/vertical orientation pairs, special sprite-base conversion, both
carried pickups and their cell cache, Mine detonation, damage, impulse, and the
Mine-to-Heart transition.

## Next boundary

The remaining `$D4F2-$D60C` selector handles cardinal contacts, animation phase,
orientation, contact auxiliary bits, and a material lookup. Its single-corner
fast path is now exact, but the rest should be represented as a separate pose
state before weapon work. State 22's bolt launcher remains the next object
family after that selector is complete.
