# Flood Milestone 84: second SDL test corrections

## Result

Milestone 84 resolves the reported intro geometry and tempo, music tempo,
missing OUCH feedback, Quiffy animation speed, and the remaining statically
identifiable sound routes. The changes are based on the recovered 68000 code
and original data rather than visual retiming alone.

## Native Bullfrog intro geometry

`BIG_PIC` has a 320-pixel backing row, but `$B46A-$B518` operates on two
160-pixel work areas. The right 160x96 area is the stable visible buffer. The
old host stretched one 160-pixel image to 320 pixels, which removed the double
image but distorted the frog.

Milestone 87 supersedes this host-framing conclusion. The animation content is
still the native right-hand 160x96 work buffer, but using that content size as
the entire SDL logical output enlarged it. It is now centred within the
320x240 PAL presentation canvas.

The animation loop has no VBlank wait. An ordinary iteration performs a CPU
copy of five planes by 76 rows by 20 bytes before processing events. A 68000
cycle estimate places that copy near 4.6-5 ms, so the SDL base step is now 5 ms
instead of 20 ms. Event iterations have varying cost; a truly cycle-exact
absolute duration would require a 68000/blitter/beam model or calibration
against an original-machine capture.

## Exact audio interrupt rate

The player setup at `$16C9A-$16CAE` enables CIA-B Timer A, writes high latch
byte `$19`, leaves the reset low byte `$FF`, and starts the timer. CIA timers
count inclusively, producing period `$19FF + 1 = $1A00 = 6656` E-clock ticks.

With the PAL CIA E-clock of 709,379 Hz, the replay rate is:

`709379 / 6656 = 106.577 Hz`

The mixer now accumulates this exact rational relationship against the output
sample rate. It does not round to an integer number of samples or milliseconds,
so there is no long-term timer drift. Existing note parsing, stable pitch
envelopes, and the two-interrupt Paula DMA attack remain intact. The regenerated
30-second WAV contains all four music channels at the corrected pitch and tempo.

## Exact OUCH feedback

The damage path at `$D186-$D1E0` tests the overlap latch at `$10DC8`. While it
is set, global sprite `$A4` is drawn at Quiffy `(x+8,y-16)`. On a clear-to-set
transition it indexes this exact 16-entry sound table:

`1,1,1,2,2,2,3,3,3,3,4,4,4,4,5,5`

The host now mirrors the current and previous latches. Continuous damage keeps
the bubble visible without restarting the voice each update. The damage test
is placed after object/Matilda dispatch, so drowning and fatal terrain do not
incorrectly produce OUCH feedback.

## Sound-dispatch closure

All 48 static calls to the original dispatcher at `$165DC` were enumerated.
Milestone 84 restores the routes that were absent from the host:

| Sound | Exact trigger |
| ---: | --- |
| 1-5 | weighted OUCH voice on damage-contact rising edge |
| 13 + 15 | paired components on dry-to-wet transition |
| 20 | parachute/balloon cancellation by terrain contact |
| 22 | diagonal slope micro-correction rising edge |
| 52 | protection/respawn timer begins at 100 with lives remaining |

Sound 22 uses the two original persistent latch words represented by a host
boolean: no repeat while a correction remains active, and re-arming only after
a frame without correction. IDs 28 and 47, initially flagged by a literal-only
scan, were already emitted by the dynamic conditional action path. No known
static dispatcher call remains unrepresented.

## Quiffy animation cadence and gameplay timing

`$DE84-$DE8E` advances Quiffy's animation by global byte `$17E86`. That byte
alternates 1,0 on successive displayed gameplay frames. The host had advanced
the phase every update; it now advances on alternating updates, matching the
same step already used by reconstructed objects and effects.

Horizontal movement remains the instruction-derived input multiplied by four,
and the SDL gameplay scheduler remains 40 ms / 25 Hz. This separates the
verified logical cadence from absolute wall-clock emulation: the original
outer loop is coupled to CPU work, Copper state, and blitter progress rather
than a single fixed delay.

## Verification

- strict C99 core build with `-Wall -Wextra -Wpedantic -Werror`: pass;
- UndefinedBehaviorSanitizer core build: pass;
- SDL Right-Ctrl/fullscreen input test, normal and sanitized: pass;
- strict SDL host syntax build with the local SDL interface: pass;
- normal and sanitized 300-tick smoke runs for all 42 levels: pass;
- exact slope, water-entry, protection, OUCH edge, alternating animation,
  intro-frame, CIA first-tick, PCM hash, and four-channel replay tests: pass.

Artifacts:

- `bullfrog_intro_milestone84.png` - native 160x96 work-buffer sequence;
- `quiffy_ouch_milestone84.png` - exact `$A4` damage bubble placement;
- `music_waveform_milestone84.png` - corrected four-channel waveform;
- `music_preview_milestone84.wav` - 30 seconds, 44.1 kHz, 16-bit stereo.
