# ReFlood — Milestone 137

## Persistent jump-and-fire control

Milestone 137 fixes the remaining flamethrower freeze after a jump-and-fire
landing. Milestone 136 allowed live input while Quiffy was airborne, but the
decision was recalculated from the current contact word on every update. As
soon as Quiffy landed, support returned and the continued-fire filter changed
the active flame into the fixed standing mode. Movement was then suppressed
until fire was released.

## Correction

Jump-and-fire is now retained as a distinct state for the complete fire hold.
On the initial fire edge, player movement runs before flamethrower State 13 is
allocated. If that update produces nonzero vertical velocity, ReFlood records
that the flamethrower began during a real jump.

The resulting mobile-fire latch:

- preserves normal airborne input;
- remains active when Quiffy lands;
- permits ordinary surface movement while fire is still held;
- clears immediately when fire is released or another weapon is selected;
- is cleared explicitly when level runtime is reset.

A flamethrower begun without a jump does not set the latch and continues to
consume movement input from its supported firing pose, preserving the standing
behaviour requested earlier.

## Verification

- standing held fire still suppresses all four movement directions;
- a real jump-and-fire edge records the mobile state;
- held input remains live throughout the airborne arc;
- after a simulated landing on full south support, held Right moves Quiffy
  immediately from x=8 to x=12 without releasing fire;
- releasing fire clears both the fire counter and mobile state;
- non-flamethrower movement, falling-beam continuity, and level-banner firing
  behaviour remain passing;
- complete core and SDL-input regression suites pass;
- strict C99 core, SDL-stub, and headless-host builds pass with warnings treated
  as errors;
- the complete core suite passes under UndefinedBehaviorSanitizer.

The package remains source-only; no executable is included.
