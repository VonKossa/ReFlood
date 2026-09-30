# ReFlood Milestone 149 Report

## Tight-corridor walking entry

Milestone 149 fixes ordinary horizontal entry into one-tile-high corridors.
Level 8 provides a clear reproduction at the long lower corridor beginning at
tile `(3,11)`, but the correction is shared by every level and contains no
level number or map-coordinate exception.

## Cause

The existing passage handoff covered two cases: turning horizontally during an
active wall climb and turning from a densely contacted vertical shaft after
the attachment cache expired. It did not cover walking horizontally into the
same opening without vertical input.

In the Level 8 reproduction, Squiffy's 16x16 collision body falls to within
eight pixels of the corridor centre and then meets its upper lip. The resulting
side contact suppresses the free-flight gravity branch. Horizontal collision
therefore remains blocked and the body never reaches the exact row needed to
enter the opening.

The original movement code at `$D99E-$DF52` has no level-specific corridor
branch. Its contact resolver, attachment cache, 16x16 collision query, and
final movement path are shared by every cavern. ReFlood's failure came from
the eligibility restriction around its reconstructed passage-alignment
handoff, not from the Level 8 map.

## Correction

An attachment-free horizontal approach can now request the same passage
alignment already used for climbing entries. The handoff still succeeds only
when all existing geometry checks pass:

- at least two rim contacts are active;
- horizontal input points toward the candidate opening;
- the short vertical alignment move is clear;
- the complete 16x16 horizontal route is clear; and
- solid terrain bounds the route immediately above and below.

These checks prevent ordinary walls, open ledges, and lone-diagonal outside
corners from activating the handoff.

## Verification

- Added an exact shipped-map regression for Level 8: Right alone aligns the
  initially offset body and continues through the corridor.
- Added synthetic left-to-right and right-to-left walking-entry regressions.
- Retained all Level 17, Level 27, Level 28, corner-cache, wall-top, and
  flamethrower movement regressions.
- Strict C99 core suite: pass.
- SDL input/settings suite: pass.
- UndefinedBehaviorSanitizer core suite: pass.
- Strict headless host build and Level 8 smoke run: pass.
- Headless load/smoke run from every level 1 through 42: pass.

The package remains source-only; no executable is included.
