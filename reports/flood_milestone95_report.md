# Flood reconstruction — Milestone 95

## Level 5 wall-rocket sound

Level 5's switch at map event 1677 has two matching trigger records. Activating
it swaps one switch cell and a six-cell payload of marker `$14` (decimal 20)
into the left wall. Marker 20 initializes six runtime State-22 bolt/rocket
records.

The original 68000 paths separate the sound sequence into two parts:

- the `$FF` switch handler requests sound 26; the sound routing table chains
  it to sound 27, producing the paired launch/flight effect;
- after a rocket collides, State 22 enters its impact/reset branch at `$13718`.
  On animation phase zero, `$1372C-$13734` requests sound 7. The remaining
  impact frames are silent, and phase seven returns the rocket to its launch
  cell.

The reconstruction already implemented the switch's 26 -> 27 launch pair but
had omitted the State-22 phase-zero sound request while translating the impact
graphics and reset. Milestone 95 restores that exact request. It does not add
an invented continuous engine sound: the original State-22 flight loop makes
no per-frame audio call.

## Regression coverage

- The single-rocket terrain-impact test now verifies no sound on the collision
  tick, sound 7 on the following phase-zero impact tick, silence on later
  impact frames, and the unchanged seven-phase reset.
- A Level 5 integration test walks the initialized player onto event 1677,
  verifies that six State-22 rockets are created and sound 26 is queued, then
  verifies six original sound-7 requests when their impact animations begin.

## Verification

- C99 `-Wall -Wextra -Wpedantic -Werror` core tests: pass
- C99 `-Wall -Wextra -Wpedantic -Werror` SDL input-stub tests: pass
- UndefinedBehaviorSanitizer core tests: pass
- UndefinedBehaviorSanitizer SDL input-stub tests: pass
- Headless load/run smoke test for all 42 levels: pass
