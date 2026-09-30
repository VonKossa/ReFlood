# Flood reverse engineering — Milestone 53

## Result

Milestone 53 replaces the provisional Mine explosion and Heart behavior with
the exact recovered 68000 control flow. It also confirms that runtime States 2,
13, and 20 share one explosion handler.

## Recovered dispatch family

| Runtime state | Role | Shipped-level reachability |
| --- | --- | --- |
| 2 | Shared explosion entry | No static or active trigger-payload instances |
| 13 | Shared explosion entry | No static or active trigger-payload instances |
| 19 | Armed Mine terrain object | 109 static markers and 5 active trigger-payload markers |
| 20 | Mine explosion | Reached when State 19 overlaps Quiffy |
| 21 | Collectible Heart | Reached when explosion phase becomes 4 |

## State 19 — Mine (`$FD28-$FD9C`)

- Tests Quiffy against the Mine rectangle `(x+4, y+12, 8, 4)`.
- Clears the Mine's terrain cell (`$E6`) on contact.
- Changes State 19 to State 20, clears animation, and offsets the explosion by
  `x -= 8`, `y -= 16`.
- Sets the three-update forced-up counter only while Quiffy's life is positive.

## States 2, 13, and 20 — shared explosion (`$1091A-$109A4`)

- If the stored phase is zero, requests sound 8 before changing the phase.
- Adds the global `$17E86` frame step, which alternates `1,0`.
- Draws `$94 + phase` while the resulting phase is below 4.
- Removes 40 life only when the resulting phase is zero and the 32x32
  explosion overlaps Quiffy's centered 16x16 collision body. Therefore a held
  phase-zero update repeats both sound and damage.
- At phase 4, changes to State 21, applies `x += 4`, sets `dy = -12`, clears
  record word `+10`, initializes the lifetime byte to 40, and draws nothing on
  that transition update.

## State 21 — Heart (`$FB42-$FC64`)

- Uses record word `+8` as a vertical phase flag. Zero accelerates upward;
  nonzero accelerates downward.
- Clamps downward speed to `+8`. Crossing below `-12` clamps to `-12` and
  enters the downward phase.
- Queries the 8x8 body at `(x+4, y+4)` using the proposed vertical movement.
- A descending collision reverses half the speed; an ascending collision
  reverses the full speed.
- Commits movement, then draws sprite `$30` at `(x+4, y+4)` and checks pickup
  overlap.
- Collection adds 10 score and 64 life capped at 511, then clears the state.
- The lifetime byte decrements every update and clears the state at zero.

The original overlap helper's internal latch has no persistent analogue in the
host because overlap tests are stateless; clearing State 21 produces the same
observable object lifecycle.

## SDL integration

States 2, 13, 20, and 21 now render through each object's recovered
`render_x/render_y` draw coordinates. The transition from explosion to Heart
suppresses a stale cached frame, matching the original no-draw branch.

## Verification

- Strict C99 build with `-Wall -Wextra -Wpedantic -Werror`: passed.
- Focused core suite: passed.
- UndefinedBehaviorSanitizer with abort-on-first-error: passed.
- All 42 decoded levels, 300 headless ticks each: passed.
- Preview regenerated from the original planar sprite/tile data with palette
  `$17F2C` and natural bitplane significance.

## Next boundary

The next useful dispatcher target is the still-inexact State 5 path at
`$13972`, followed by the remaining generic State 6/15 effects and the
unresolved Sparkling Fungi hazard.
