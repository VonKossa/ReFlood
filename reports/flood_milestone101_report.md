# Flood reconstruction — Milestone 101

## Doctor Dusty's downward dynamite drop

Doctor Dusty's dynamite appeared to be inverted in Level 15 because its entire
trajectory moved upward instead of dropping toward the floor. The sprite data
itself was upright; the inversion came from the vertical-acceleration branch.

The original State-10 routine at `$13890` executes `TST.W` on runtime-record
offset `+8`. That word aliases two adjacent bytes:

| Record byte | Meaning when the dynamite starts |
| --- | --- |
| `+8` | Direction/phase flag, initially zero |
| `+9` | Animation and lifetime countdown, initially 20 |

Because byte `+9` is nonzero, the complete big-endian word at `+8` is nonzero
and the original takes the positive-Y acceleration path. The reconstruction
tested only byte `+8`, saw zero, and incorrectly accelerated toward negative Y.

Milestone 101 restores the word-sized test. A newly emitted stick now moves
down by 1, then 2, then 3 pixels on its first updates, while collision,
animation, expiry, and the following State-9 explosion remain unchanged.

## Regression coverage

- the same-pass Doctor Dusty emission moves the dynamite one pixel downward;
- the next two updates continue the original `+1,+2,+3` acceleration sequence;
- the animation frames, render offsets, and countdown advance in lockstep;
- the offset-`+8` direction byte and offset-`+9` countdown alias remains tested
  when vertical speed is negative and positive;
- the landing collision and State-9 explosion transition remain covered.

## Verification

- C99 `-Wall -Wextra -Wpedantic -Werror` core tests: pass
- C99 `-Wall -Wextra -Wpedantic -Werror` SDL input-stub tests: pass
- UndefinedBehaviorSanitizer core tests: pass
- UndefinedBehaviorSanitizer SDL input-stub tests: pass
- Headless load/run smoke test for all 42 levels: pass
