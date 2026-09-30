# ReFlood — Milestone 119

## Ceiling-turn delay

The reported Up-to-Right delay was reproduced with Quiffy attached beneath a
ceiling tile detected by one cardinal north probe. The attachment helper used
only `contact_count <= 1` to identify an outer corner and cleared horizontal
surface input. Because the cardinal ceiling was misclassified as a diagonal
corner, Right remained ineffective until the attachment timeout advanced.

## Correction

Attachment maintenance now checks both contact count and direction:

- a lone cardinal north or south contact preserves live horizontal input;
- a lone cardinal east or west contact preserves live vertical input;
- only a lone diagonal contact suppresses the tangent while the retained
  perpendicular attachment direction carries Quiffy around the corner.

This removes the ceiling delay without weakening actual corner transitions.
The distinction also makes narrow floor and wall surfaces symmetric.

## Verification

- strict C99 core, SDL-stub, and headless builds with warnings as errors: pass;
- complete existing gameplay and presentation regression suites: pass;
- one-contact ceiling accepts Right on the first update: pass;
- first ceiling movement step is the normal four pixels: pass;
- one-contact floor accepts the symmetric Right command immediately: pass;
- lone diagonal contact retains corner-carry behavior: pass;
- milestone-118 full jumps over both sides of a block remain exact: pass;
- jump-and-fire and falling flamethrower regressions remain exact: pass.
