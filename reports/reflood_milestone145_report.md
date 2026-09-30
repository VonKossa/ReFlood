# ReFlood — Milestone 145

## Level 17 extra-life passage

Milestone 145 fixes the one-tile opening leading to the extra-life room in
Level 17.

The room's extra life is at tile `(49,32)`. Its actual entrance is the
one-cell gap at `(51,32)`, approached from the east while Squiffy climbs the
wall. Milestone 141 claimed coverage for this entrance, but its regression
placed Squiffy's collision core at column 49—already inside the room—and moved
right. It therefore tested ordinary movement within the chamber rather than
entry through the doorway.

## Cause and correction

The Milestone 141 handoff recognized a 16-pixel-high corridor only when
Squiffy's 16-pixel collision core was already aligned to the exact same pixel
row. At four pixels per gameplay update, that left a single update in which a
horizontal turn could be accepted. Pressing toward the opening just after that
update retained the wall-climb direction and carried Squiffy past the gap.

During an active wall climb, the tight-passage handoff now checks the nearest
16-pixel row, at most eight pixels away. It accepts the alignment only when:

- at least two contacts still identify the passage rim;
- horizontal input points into the wall/opening;
- the short vertical alignment move is collision-free;
- the complete 16×16 horizontal route is clear; and
- solid terrain bounds that route immediately above and below.

On success Squiffy is centered vertically in the opening, the stale climb
attachment is released, and the commanded horizontal four-pixel step enters
the passage. The restrictions preserve ordinary wall movement and the
recovered convex-corner cache.

## Verification

The corrected shipped-map regression now:

- starts outside the real Level 17 doorway at column 52;
- climbs past the exact-alignment update before Left is pressed;
- verifies Squiffy is captured at row 32 instead of continuing upward;
- crawls through the doorway and collects the extra life at `(49,32)`;
- verifies that the pickup is removed and the lives counter increases.

The complete core and SDL-input suites pass, including all prior corner and
passage regressions. Strict core, SDL-stub, and headless-host builds pass with
warnings treated as errors; the core suite also passes under
UndefinedBehaviorSanitizer. The package remains source-only; no executable is
included.
