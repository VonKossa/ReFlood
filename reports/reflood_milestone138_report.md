# ReFlood — Milestone 138

## Exact jump-and-fire phases

Milestone 138 corrects the excessive movement freedom introduced by milestones
136 and 137. The original behaviour distinguishes the airborne part of a
jump-and-fire sequence from its eventual ground landing.

While fire remains held:

- the first fire edge can initiate the jump;
- directional input cannot steer Quiffy during the airborne arc;
- stored vertical momentum continues upward and gravity naturally brings him
  down again;
- contact with a ceiling keeps Quiffy fixed there while the flamethrower
  continues;
- after landing on the ground, normal movement becomes available without
  releasing fire.

## Correction

ReFlood still records whether the flamethrower began with a real jump, but the
latch no longer grants unrestricted movement. Continued fire now suppresses
input unless that jump-started sequence has reached stable south support.

This makes ground contact the only transition into post-landing movement.
North/ceiling contact, wall contact, and the contact-free airborne state retain
the firing lock. A standing-started flamethrower never sets the jump latch and
therefore keeps its existing fixed-pose behaviour.

## Verification

- standing held fire suppresses all four movement directions;
- the initial jump-and-fire edge produces `(dx,dy) == (4,-16)`;
- on the following airborne update, held Right+Up is consumed, x remains fixed,
  and vertical momentum advances from `-16` to `-13`;
- through the complete natural arc, x remains fixed, velocity crosses the apex,
  gravity begins the descent, and the flame stays active;
- the natural ground landing restores Right movement on the next update while
  fire remains held;
- a dedicated ceiling geometry stops the jump and continued Right+Up cannot
  move Quiffy away while firing;
- falling-beam continuity, non-flamethrower movement, and level-banner firing
  behaviour remain passing;
- complete core and SDL-input regression suites pass;
- strict C99 core, SDL-stub, and headless-host builds pass with warnings treated
  as errors;
- the complete core suite passes under UndefinedBehaviorSanitizer.

The package remains source-only; no executable is included.
