# ReFlood — Milestone 147

## Level 28 narrow-shaft passage

Milestone 147 fixes the circled one-tile opening in Level 28. The opening is
tile `(2,22)`, entered to the right from the vertical shaft at column 1.

## Reproduced failure

Unlike the Level 17 entrance fixed in Milestone 145, the Level 28 approach is
a one-tile-wide shaft. Squiffy's centred collision body touches both walls
while climbing and produces contact masks with five or six active rim points.

The original ten-update corner cache expires well before Squiffy reaches row
22. Because the dense contact ring contains more than two points, the cardinal
resolver does not create a new cache. ReFlood's passage assist incorrectly
required `attachment_mode == 2`, so it was unavailable precisely when the
opening was reached. With Up+Right still held, the final diagonal collision
discarded horizontal motion and Squiffy continued climbing past the opening.

## Correction

The passage handoff now recognizes two valid climb sources:

- the existing active wall cache; or
- an expired cache in a dense shaft, identified by more than two contacts,
  no active attachment, and continuing vertical input.

This second source does not bypass the Milestone 145 geometry checks. Entry is
accepted only when the short vertical alignment is collision-free, the full
16×16 horizontal route is clear, and solid terrain bounds the candidate route
immediately above and below. Lone-diagonal outside corners and ordinary walls
therefore cannot activate it.

## Verification

The new shipped-map regression:

- loads Level 28 and starts below the marked opening at column 1;
- climbs long enough for the ten-update attachment cache to expire;
- retains the real five/six-contact narrow-shaft pattern;
- holds Up+Right continuously;
- enters tile row 22 with horizontal velocity 4 and vertical velocity 0; and
- crawls right through the passage instead of continuing up the wall.

All prior Level 17 and Level 27 passage tests and the complete corner-movement
suite remain in place. The complete core and SDL-input suites pass; strict
core, SDL-stub, and headless-host builds pass with warnings treated as errors;
and the core suite passes under UndefinedBehaviorSanitizer. The package remains
source-only; no executable is included.
