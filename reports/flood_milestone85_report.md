# Flood Milestone 85: flamethrower and presentation corrections

## Result

Milestone 85 restores the missing sustained-flame segment and the complete
malfunction chicken, corrects the Bullfrog animation cadence and missing music
lead-in, and keeps fullscreen active throughout presentation resolution
changes.

## Complete sustained flamethrower

The original draw path at `$D0CC-$D160` handles the held weapon separately
from the action beam. When fire phase D5 reaches three, `$D0F4-$D12C` first
draws another 32x32 global sprite at Quiffy's Y and at X -32 or X +32:

| Direction | Normal tip | Position |
| --- | ---: | ---: |
| left | `$D0` | Quiffy X - 32 |
| right | `$D1` | Quiffy X + 32 |

The host previously drew the `$C8-$CF` Quiffy fire pose and `$37-$3A` beam but
omitted this intermediate piece, producing the reported gap. It is now
composited immediately before Quiffy's main firing sprite, matching the
original ordering.

## Malfunction chicken

State 13 calls `$D250`, which has a one-in-sixteen failure result. It selects
two offsets according to horizontal direction and advances the action to State
15. `$D0CC-$D160` applies those offsets only after fire phase three:

| Direction | Tip replacement | Main-pose replacement | Composite |
| --- | ---: | ---: | --- |
| left | `$DA` | `$DB` | `$DA+$DB` |
| right | `$DD` | `$DC` | `$DC+$DD` |

These are the two complete chicken orientations in the original sprite bank.
State 15 emits no flame beam. Existing sound 28 remains unchanged. Releasing
fire, losing support, or entering death clears the action, pose flag, and both
offsets.

## Bullfrog cadence and music lead

Music setup `$1647A` returns immediately to `$98F8`, which then enters `$B46A`.
`$B46A` loads the 100,000-byte `BIG_PIC`, creates its mask, installs the display,
and only then begins the visible animation. The original floppy transfer made
the music audibly precede the picture; instant pre-extracted host loading had
removed that interval.

The SDL startup now displays black and services input/fullscreen for four
seconds after starting music and before presenting the first Bullfrog frame.
This models the original media-loading interval without changing the exact
106.577 Hz CIA replay rate.

Milestone 84's 5 ms animation step was also too short. In an ordinary `$B870`
iteration, five planes each copy 76 rows of 20 bytes through paired `movem.l`
instructions. Those copies alone require approximately 6.8 ms on a 7.09 MHz
68000, before five blitter waits, loop setup, input, and scheduled blits. The
host step is now 10 ms, producing approximately 12.16 seconds for all 1216
steps. Absolute disk and CPU timing remains hardware/media dependent; the new
values reproduce the missing ordering and respect the measured instruction
lower bound.

## Fullscreen presentation continuity

F already reached the intro and title input loops, but each following logical
resolution change called `SDL_SetWindowSize`. Some SDL backends interpret that
while desktop fullscreen is active by restoring a window, making fullscreen
appear usable only after gameplay begins.

Fullscreen state is now shared by the input and presentation setup paths.
Every stage reapplies desktop fullscreen when active and skips window resizing;
otherwise it applies the stage's normal 2x window size. SDL logical rendering
continues to preserve aspect ratio and letterbox unused space. F remains a
literal character on the password and copy-protection text-entry screens, so
all original answers remain enterable; fullscreen selected earlier remains
active through both.

## Verification

- exact normal tip IDs `$D0/$D1` and malfunction pairs `$DA+$DB/$DC+$DD`: pass;
- one-in-sixteen State-15 route and sound 28: pass;
- 4,000 ms music-lead scheduler test: pass;
- fullscreen reapplication and window-resize suppression test: pass;
- strict C99 core and SDL interface builds with warnings as errors: pass;
- core and SDL tests under UndefinedBehaviorSanitizer: pass;
- normal and sanitized 300-tick smoke runs for all 42 levels: pass.

Preview artifacts:

- `flamethrower_chicken_milestone85.png`;
- `bullfrog_intro_milestone85.png`.
