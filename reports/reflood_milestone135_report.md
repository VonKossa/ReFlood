# ReFlood — Milestone 135

## Direction-gated corner attachment

Milestone 135 fixes the over-aggressive corner grab reported at the Level 4
starting platform. When Quiffy jumped toward the platform's right underside
corner while travelling left, an immediate Right input could be ignored for
roughly ten gameplay updates and Quiffy continued left instead.

## Original-code comparison

A natural-input trace was taken through the original joystick poll and contact
resolver, without pre-seeding the attachment cache. At the relevant Level 4
state, both versions reached position `(628,440)` with velocity `(-4,0)` and
contact mask `$81` (north plus north-west).

The original `$D9F8-$DA48` path then checked the missing north-east diagonal.
Because the horizontal input pointed left, away from that exposed east end, it
returned at `$DAEA` and left the attachment mode clear. Right input on the next
update therefore moved Quiffy immediately to `(632,440)`.

ReFlood performed only the conditional tangent assignment and then fell
through to activate attachment mode unconditionally. Its cached left direction
overrode subsequent Right input until the cache expired.

## Correction

The missing early-return semantics have been restored for every cardinal
orientation:

- a north or south surface attaches at an exposed west/east end only when the
  horizontal input points toward that end;
- an east or west surface attaches at an exposed north/south end only when the
  vertical input points toward that end;
- fully supported cardinal surfaces and genuine toward-corner movement retain
  their original attachment behaviour.

This is a resolver correction rather than a new cancellation rule. Inputs are
tested at the exact point where the original decides whether an attachment
cache should be created, so an away input is never replaced by the wrong
cached direction.

## Verification

- all eight exposed surface-end orientations attach for toward-corner input;
- all eight orientations return without attachment for away input;
- the exact Level 4 reproduction accepts Right on the next update and moves
  from `(628,440)` to `(632,440)`;
- the existing live-derived Level 13 outside-corner trace remains passing;
- the complete core and SDL-input regression suites pass;
- strict C99 core, SDL-stub, and headless-host builds pass with warnings treated
  as errors;
- the complete core suite passes under UndefinedBehaviorSanitizer.

The package remains source-only; no executable is included.
