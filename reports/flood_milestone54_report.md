# Flood reverse engineering — Milestone 54

## Result

Milestone 54 reconstructs runtime State 5 Beady Ball instruction-for-
instruction from `$13972-$13A4A` and integrates its fixed `$AE` sprite into
the SDL renderer. During the trace, the overlapping byte/word semantics at
object record offset `+8` exposed and corrected a State-21 Heart error carried
by Milestone 53.

## Marker and reachability

Marker 5's initializer at `$E896-$E8B2` allocates State 5, sets `dx=4` and
`dy=16`, clears the map marker, and links the object into the active list.

The recovered 42-level dataset contains zero static marker-5 cells. Inspection
of every rectangle addressed by every active trigger record also finds zero
marker-5 payload bytes. State 5 is executable and synthetically initializable,
but dormant in the shipped levels. Unreferenced byte value 5 in unused payload
storage is not counted as a reachable object.

## Exact State-5 handler

The handler loads `(x,y,dx,dy)` from record offsets `+0..+6` and tests the full
word at `+8`:

- word `+8 == 0`: subtract 1 from `dy`;
- word `+8 != 0`: add 1 to `dy`;
- clamp `dy` above to `+16`;
- when `dy < -12`, clamp to `-12` and write word value 1 at `+8`.

It calls swept collision routine `$F0E0` for a 32x32 body at `(x,y)` with the
proposed `(dx,dy)`:

- horizontal solid contact negates `dx`;
- ascending vertical contact negates the complete upward speed;
- descending vertical contact halves the speed; a halved result of 2 or less
  is replaced by 12; the result is negated and word value 1 is written at
  `+8`.

The adjusted velocity is always committed. The object then draws fixed 32x32
sprite `$AE` at its new position. A 32x32 overlap against Quiffy's centered
16x16 body removes 8 life after movement.

## Big-endian record alias correction

The original record stores byte fields at `+8` and `+9`, but State 5 and State
21 also perform word operations beginning at `+8`. On the 68000,
`move.w #1,$8(a0)` writes byte `+8 = 0` and byte `+9 = 1`.

The host now models that alias explicitly through word-test and word-write
helpers. For State 5 the phase marker therefore resides as value 1 in byte
`+9`. For State 21, initial lifetime 40 in byte `+9` makes the tested word
nonzero, so the Heart immediately applies gravity to `dy=-12`. A descending
collision writes word 1; the final decrement changes byte `+9` from 1 to 0 and
clears State 21 on the same update. This supersedes the earlier Milestone-53
description of an independent two-phase Heart flag.

## Focused verification

Tests cover free initial motion, the `-12` upward clamp and exact word-1 write,
subsequent downward acceleration, horizontal reflection, normal half-speed
floor rebound, special minimum `-12` rebound, full-speed ceiling reflection,
fixed `$AE` rendering, 8-point post-movement damage, corrected State-21
launch gravity/collision expiry, and zero State-5 reachability across all 42
static maps and active trigger rectangles.

Strict C99 compilation with `-Wall -Wextra -Wpedantic -Werror`, the complete
core suite, UndefinedBehaviorSanitizer with abort-on-first-error, and all 42
levels for 300 headless ticks each pass.

## Next boundary

The next bounded target is shared handler `$10A10`, used by runtime States 6,
9, and 15. State 9 is already present for Doctor Dusty's companion, but the
full shared dispatch and reachability of States 6 and 15 need reconciliation.
