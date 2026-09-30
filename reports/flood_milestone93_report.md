# Flood Milestone 93: exact reverse marker scan and Level 5 start

## Result

Milestone 93 fixes Level 5 opening with only its HUD and a wall visible. Quiffy
now starts in the intended upper-left cavern and appears at the normal camera
anchor when play begins.

## Cause

Level 5's 128x100 map contains byte value 22 at two positions:

- index 900, tile `(4,7)`;
- index 9807, tile `(79,76)`.

A marker 23 at `(85,40)` sets the small-cavern camera maxima to `(1040,432)`.
The former ascending host scan processed `(4,7)` first and `(79,76)` last, so
the latter overwrote Quiffy's position. Camera Y then clamped to 432 while
Quiffy remained at world Y=1216, far below the 208-line viewport.

## Original scan order

The original initializer at `$E736-$E75C` sets D1 to `$31FF`, addresses the
map as `base+D1`, and ends its loop with `DBF D1`. Marker initialization is
therefore strictly descending from index 12799 to zero.

The reconstruction now uses that order. In Level 5, `(79,76)` is visited first,
the bounds marker is applied next, and `(4,7)` is visited last. Quiffy starts at
world `(64,112)`; camera `(0,20)` places him at visible `(64,92)`.

Descending order also determines which of several marker-21 water sources is
retained for trigger-driven flood rebuilding. Level 30 now correctly retains
the lowest-index of its three adjacent sources at world `(720,1552)`.

## Verification

- focused Level 5 start, bounds, camera, and visible-position regression: pass;
- reverse-order Level 30 multi-source water regression: pass;
- strict C99 core and SDL-interface builds with warnings as errors: pass;
- core and SDL-interface tests under UndefinedBehaviorSanitizer: pass;
- all 42 cavern headless smoke runs: pass.
