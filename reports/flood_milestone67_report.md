# Flood Milestone 67: frame-accurate water integration

## Result

Milestone 67 closes the timing boundary left by the exact propagation engine.
The SDL renderer now displays the secondary-map state that existed before the
current frame's propagation work, while gameplay and the propagation engine
continue using the live map. The Quiffy water/air path at `$DC4A-$DEF2` was
audited instruction by instruction and four inherited host discrepancies were
corrected.

## Visible map versus live map

The original order is explicit in the frame routine:

1. `$AEF0-$AF2A` selects the current `$0A80` W_BLOCKS bank and composites the
   secondary map through `$ED66`.
2. `$AF2E-$AF58` applies the pause or calls `$F53A` `speed²` times.
3. `$AF60-$AF74` applies alternating-buffer speed decay.
4. `$AF76` begins the transparent HUD compositor call.

The compact SDL loop updates game state before presenting its texture. Calling
the propagation scheduler directly from that tick therefore exposed the new
state one frame early. `FloodWorld` now holds:

| Map | Consumer |
| --- | --- |
| `water` | Quiffy, mechanisms, triggers, selector, propagation |
| `render_water` | `$ED66`-equivalent W_BLOCKS compositor only |

At the end of a host tick, `water` is copied to `render_water` and only then
advanced. Standalone level loading initializes both maps identically. The
alternating `render_buffer_index` still selects W_BLOCKS bank 0 or 1 for the
preserved visible state.

## Quiffy buoyancy at `$DC4A-$DCB2`

With no surface contacts, `$109A6` is called twice at Quiffy's centre and one
pixel above it. Each call converts the secondary state through the exact
40-byte fill-height table. The low nibble of Y plus that fill determines a
`+1` or `-1` buoyancy contribution; the combined vertical velocity then moves
one unit toward zero. The host already represented these two probes.

The later `$DD58` centre probe was incorrect in the host. The original forces
vertical speed 2 only when all three conditions hold:

- water fill is nonzero;
- the low eight contact bits are zero;
- raw vertical input is positive (down).

The prior host did so when vertical input was zero. The condition is now
correct, and ordinary velocity is clamped to the original `-16..+16` interval.

## Air and drowning at `$DEB4-$DEF2`

The breathing probe is at Quiffy `(+8,+8)`, not `(+8,+12)`. Its exact rules
are:

| Centre fill | Display buffer | Result |
| ---: | ---: | --- |
| 8 or more | 0 | no air change |
| 8 or more | 1 | subtract 1 air; if the result is negative, subtract 4 Life Force immediately |
| below 8 | either | if air is below 63, add 2 |

Because the signed test follows the subtraction, air 0 becomes -1 and damages
Life Force on that same buffer-1 update. The original `+2` recovery can move
air 62 to 64; it is not clamped after the addition. The host preserves these
word-level semantics. The water-specific pose selection now uses this same
centre coordinate.

## Original-level integration cases

Level 30 contains three adjacent marker-21 sources at `(45,97)`, `(46,97)`,
and `(47,97)`. Initial loading seeds all three. Since the original stores only
one coordinate pair, the last marker wins; a trigger rebuild clears the first
two and replays from `(47,97)`.

Level 36 places its source at `(0,1)`. `$F5E8` literally tests saved source X,
so a rebuild treats this as absent and clears the flood. This original quirk
remains covered rather than silently repaired.

## Verification

- strict C99 with `-Wall -Wextra -Wpedantic -Werror`: pass;
- complete core regression: `core ok`;
- UBSan with abort on first error: pass;
- normal and UBSan headless hosts complete all 42 levels;
- consecutive ticks prove visible states 4 and 8 while live states have
  already advanced to 8 and 12 respectively;
- Quiffy tests distinguish live water from the render snapshot and cover
  alternate-buffer air drain, air-zero immediate damage, `+2` recovery, and
  unsupported DOWN speed 2;
- exact `$E4/$E5`, trigger replay, Level 30, and Level 36 cases remain green.
