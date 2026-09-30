# ReFlood — Milestone 142

## Space Hopper floor alignment

Milestone 142 corrects the mounted Space Hopper's vertical alignment. In Level
27, activating the Hopper and remaining seated now places its artwork on the
same floor line as the original game instead of slightly below it.

The ordinary Quiffy contact ring is a centred 16×16 body at sprite offset
`(+8,+8)`. ReFlood incorrectly reused that geometry for the Space Hopper.
Because the mounted sprite is 32 pixels high, its support probe did not see the
floor until the artwork had descended too far.

The original State-1 path is explicit:

- `$101EA` writes collision height `32`;
- `$101F2` writes vertical offset `0`;
- `$101FA` calls the ordinary contact-ring builder;
- `$10240-$10248` restore height `16` and offset `8` before normal
  movement continues.

ReFlood now parameterizes the shared contact-ring reconstruction and uses the
temporary `16×32` body at offset `(+8,0)` only for mounted Space Hopper
support detection. Normal Quiffy movement, collision, corner assistance, and
the Hopper's subsequent ordinary movement collision remain unchanged.

## Verification

The regression suite now checks both an isolated floor and the shipped Level 27
Hopper at tile `$DC`:

- the full-height mounted body detects support where the normal body does not;
- the Level 27 activation settles at the recovered original vertical position;
- the Hopper remains in State 1 with zero vertical velocity and south support;
- the pre-fix milestone fails the new support assertion;
- complete core and SDL-input regression suites pass;
- strict C99 core, SDL-stub, and headless-host builds pass with warnings treated
  as errors;
- the complete core suite passes under UndefinedBehaviorSanitizer.

The package remains source-only; no executable is included.

