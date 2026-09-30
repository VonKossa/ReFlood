# ReFlood — Milestone 126

## Level 13 outside-corner handoff

The reported reproduction is now covered directly: Level 13, hold Up, then add
Right without releasing Up while climbing around an outside 90-degree corner.

### Diagnosis

The original `$F0E0` collision routine was disassembled again under emulation.
Its inclusive extent calculation is already matched by ReFlood, ruling out the
suspected width/height off-by-one.  A trace using the real Level 13 map then
showed that milestones 121, 123, and 125 all produced the same visible path at
the tested corner despite different internal cache states.

At map cell `(5,46)`, ReFlood changed from the full east-wall contact ring to
east plus south-east contact four updates before it allowed horizontal motion.
The missing north-east probe had already identified the convex wall end, but
the ordinary swept X collision continued to reject Right until the entire
16-pixel collision core reached exact clearance.

### Fix

Milestone 126 adds a narrowly scoped host corner-handoff bridge:

- it accepts only an exact cardinal-wall plus trailing-diagonal contact pair;
- both the inward wall direction and the tangential direction must be held;
- all four wall-end orientations are represented;
- the player advances to the corresponding lone-diagonal handoff position;
- the destination contact is rebuilt and must be exactly the expected lone
  diagonal, otherwise the complete prior player and contact state is restored;
- the existing attachment maintenance and contact resolver perform the turn.

This avoids the broad cached-direction replay from milestone 123 and therefore
does not suppress fresh input on ordinary surfaces.

### Verification

- actual Level 13 `(5,46)` Up-then-Up+Right trace: turn advances four movement
  updates and enters the lone south-east contact state;
- actual Level 13 `(42,80)` equivalent corner: same early handoff;
- Level 13 regression asserts position `(60,712)`, contact `$08`, and the wall
  attachment after the first approach update;
- full core regression suite: pass;
- SDL input/settings stub suite: pass;
- headless Level 13 run: pass.

The build environment did not provide CMake, so the same C99 source sets used
by the CMake targets were compiled directly with strict warnings for these
checks.
