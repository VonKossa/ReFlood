# ReFlood — Milestone 136

## Airborne flamethrower control

Milestone 136 fixes the movement freeze caused by holding fire after starting
a jump with the flamethrower.

The milestone 117/118 input filter correctly left the first fire edge alone so
Up and fire could begin together. On every later held-fire update, however, it
zeroed both joystick axes solely from the selected weapon and fire counter.
That rule also applied while Quiffy was airborne, removing horizontal control
for the entire jump or fall until fire was released.

## Correction

Continued flamethrower fire now consumes directional input only while Quiffy is
on a surface. Airborne movement remains live when either:

- the current contact word contains no surface contact; or
- vertical velocity is already nonzero.

The velocity condition is important on the update immediately after takeoff.
At that point the stored contact word still describes the supporting surface
from the preceding collision pass, but `dy == -16` proves that the jump has
already started.

This retains all earlier flamethrower behaviour: standing fire locks movement,
the initial fire edge can start a jump, the beam initializes from its remembered
takeoff support, and an established flame continues when a bridge retracts.

## Verification

- standing held fire still suppresses Left, Right, Up, and Down;
- non-flamethrower weapons still accept movement while firing;
- jump plus flamethrower starts with velocity `(4,-16)` in the focused test;
- the next held-fire airborne update retains Right+Up, moves from `(12,120)`
  to `(16,107)`, and advances vertical velocity from `-16` to `-13`;
- an established flame still survives removal of its supporting floor and
  follows Quiffy while he falls;
- the level-banner gameplay update retains the supported movement lock;
- complete core and SDL-input regression suites pass;
- strict C99 core, SDL-stub, and headless-host builds pass with warnings treated
  as errors;
- the complete core suite passes under UndefinedBehaviorSanitizer.

The package remains source-only; no executable is included.
