# ReFlood — Milestone 146

## Exact Space Hopper movement body

Milestone 146 replaces the earlier Space Hopper alignment workaround with the
complete original State-1 collision-geometry lifecycle.

## Original-code finding

The original auxiliary routine at `$101EA-$10380` does more than temporarily
use a tall body for its support probe:

- `$101EA` sets collision height to 32;
- `$101F2` sets vertical collision offset to 0;
- `$101FA` builds the mounted contact word;
- the ordinary Quiffy movement and collision pass then runs while those
  geometry values remain active.

The normal `16×16` height and `+8` offset are restored explicitly at
`$10240-$10248` only on the fatal transition to auxiliary State 2. They are
also reset at the start of the next ordinary gameplay update before State 1
sets them again.

ReFlood had applied `16×32/+0` only while building State-1 contacts. Its final
movement sweep always used Quiffy's centred `16×16/+8` body. Consequently:

- Down could move the mounted player four pixels into the floor before the
  smaller body became blocked;
- support and movement disagreed about the resting Y coordinate; and
- Milestone 144 needed a render-only `-4` offset to conceal the initial
  mismatch, exposing a second apparent resting height after small bounces.

## Correction

State 1 now carries an explicit geometry flag into the complete movement
resolver. Both the first collision query and its velocity-reduction loop use
the mounted `16×32/+0` body. The fatal transition restores ordinary geometry
for that same update, matching the original instructions, while a successful
dismount retains mounted geometry until the current update finishes.

The render-only offset from Milestone 144 is removed. On the Level 27 floor at
`y=176`, the 32-pixel Hopper sprite and its movement body both begin at
`y=144`; presentation and physics therefore share one coordinate.

## Verification

The Level 27 regression now verifies that:

- neutral State 1 settles at `y=144` with south support and zero velocity;
- holding Down cannot lower the Hopper below `y=144`;
- a short Up bounce leaves the support line and returns to exactly `y=144`;
- no render offset or second resting coordinate remains;
- State 1 and its mounted pose remain active after landing.

All prior movement, corner, passage, item, and auxiliary-state regressions are
retained. The complete core and SDL-input suites pass; strict core, SDL-stub,
and headless-host builds pass with warnings treated as errors; and the core
suite passes under UndefinedBehaviorSanitizer. The package remains source-only;
no executable is included.
