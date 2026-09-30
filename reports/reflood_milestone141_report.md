# ReFlood — Milestone 141

## One-tile passage entry

Milestone 141 fixes Quiffy becoming unable to turn from a wall climb into a
one-tile-high horizontal opening.

The collision body itself was already correct: the original movement path uses
the centred 16×16 core at sprite offset `(+8,+8)`. The blocker was the
host-side wall-attachment handoff. At the mouth of a tight passage, the cached
vertical climb direction could survive after the horizontal route became
clear. The later diagonal-corner test then preferred the stale vertical motion
and cancelled horizontal movement, carrying Quiffy past the opening.

When Quiffy is wall-attached and the player commands movement into the wall,
the game now tests the cardinal horizontal route. It releases only the cached
vertical component when:

- the horizontal 16×16 route is clear;
- solid terrain bounds that route immediately above and below, identifying a
  genuine one-tile-high passage; and
- at least two rim contacts are present.

The restriction is intentional. Lone-diagonal contacts still use the recovered
ten-update convex-corner carry, so the Level 13 outside-corner behaviour and
the later direction-reversal fixes are unchanged.

## Shipped-level regressions

The core suite now exercises the real level maps in addition to mirrored
synthetic geometry:

- both affected narrow openings in Level 17 accept the horizontal handoff;
- the Level 27 alcove at row 41 can be entered while climbing and its button
  cell at column 37 is reached by the ordinary item dispatcher;
- left-to-right and right-to-left passage entry both work;
- the existing original corner traces continue to pass without changed
  expectations.

## Verification

- complete core regression suite passes;
- complete SDL-input regression suite passes;
- strict C99 core, SDL-stub, and headless-host builds pass with warnings treated
  as errors;
- the complete core suite passes under UndefinedBehaviorSanitizer.

The package remains source-only; no executable is included.

