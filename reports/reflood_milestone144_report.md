# ReFlood — Milestone 144

## Space Hopper render anchor

Milestone 144 completes the Space Hopper floor-alignment correction in Level
27.

Milestone 142 restored the original State-1 support probe: a temporary
`16×32` body at sprite offset `(+8,0)`. That correction moves the mounted
physics position from `y=152` to `y=148` on the Level 27 floor whose top is
`y=176`. Testing nevertheless showed that the artwork remained too low.

The remaining discrepancy was in the host compositor. It drew Hopper frames
`$AE-$B5` at the generic Quiffy visual origin. Those frames are 32 pixels high,
so drawing from `y=148` extends them to `y=180`, four pixels through the floor
line. Mounted State 1 now uses a render-only Y offset of `-4`; the frame spans
`y=144..175` and its lower edge meets the floor at `y=176`.

This does not alter `player.y`, collision queries, velocity, bounce behavior,
normal Quiffy frames, dismounting, or the separate State-2 effect renderer.

## Verification

The regression suite now verifies that:

- the restored full-height Hopper support body still settles at `player.y=148`;
- the mounted render origin is four pixels higher;
- the 32-pixel frame bottom exactly equals the Level 27 floor line;
- the render offset is absent outside mounted State 1;
- complete core and SDL-input regression suites pass;
- strict C99 core, SDL-stub, and headless-host builds pass with warnings treated
  as errors;
- the complete core suite passes under UndefinedBehaviorSanitizer.

The package remains source-only; no executable is included.
