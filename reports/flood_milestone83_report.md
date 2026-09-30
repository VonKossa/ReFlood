# Flood Milestone 83: first SDL test corrections

## Result

Milestone 83 converts the first real SDL-machine test report into eight focused
binary corrections. The fixes cover the language selector, protection random
challenge, intro presentation, gameplay cadence, Quiffy facing and pose-bank
selection, the level-start banner, and three errors in the software audio
translation.

## Language selection and protection challenge

At `$125FE`, the original adds signed joystick X `$17E78` to the persistent
language word `$126EC`, waits for horizontal release, and masks the result with
3. The confirmation test at `$A958` is separate. The host now changes language
on each new left/right edge, without requiring Right Ctrl, and suppresses
auto-repeat while the direction remains held.

The original copies beam position `$DFF006` to RNG word `$17E90` at `$13EBE`
after fire release, then `$D77C` advances it as:

`seed = seed * $24A1 + $24DF` (low 16 bits)

The SDL host seeds that step with the low 16 bits of `SDL_GetTicks()` at the
same release boundary. The core retains an explicit seed entry point so its
challenge tests remain deterministic.

## Bullfrog intro presentation

`$B4FA-$B516` maintains a second 160x76 working copy during the scheduled blit
animation. Treating both halves as independent visible pictures produced the
reported side-by-side duplicate. The presentation pixel path now maps the
160-pixel animation once over the 320-pixel logical output. All 204 event
records and the 1216-frame presentation duration remain unchanged.

## Quiffy sprite paths

The movement-facing test at `$DE34-$DE4A` is literal:

| Horizontal velocity | Horizontal bank |
| --- | ---: |
| negative / left | 0 |
| positive / right | 4 |

The host previously used the opposite mapping. `$DF54` also clears the current
pose, both direction banks, and animation phase. The initial host pose `$55`
was therefore wrong.

The drawing path at `$CE80-$D17A` uses global sprite `$50 + D4F2` for ordinary
movement. `$C8-$CF` is a separate four-frame-per-direction firing bank selected
only by the weapon pose. Correcting this distinction removes the flamethrower
fragment that appeared during ordinary movement.

## Gameplay and level-start cadence

The original outer gameplay loop is hardware/blitter limited rather than
explicitly VBlank-waited. The 51-count banner and the observed machine speed
establish the host cadence at 25 updates per second. SDL gameplay and banner
steps now use 40 ms scheduling, while presentation animation and the audio
interrupt model remain at 20 ms / 50 Hz.

The earlier reconstruction accumulated the initializer X and produced a slide.
Instruction `$AFA8` instead reloads D4 from `$6A` on every banner frame. The
host now draws `LEVEL nn` at fixed X=106 for all 51 frames, lasting 2.04 seconds.
The call at `$AD7C` to `$E232` with sound ID 7 is issued immediately before the
banner, restoring the spoken “Let's go” cue.

## Audio-driver corrections

The audit of `$17014-$17530` found three translation errors:

1. `$1727E` reloads the selected delay before the next track parse. Without
   this reload, later notes could run at one note per 50 Hz interrupt.
2. `$17452-$1745E` evaluates the pitch envelope from the unchanged note period
   stored at channel offset `$20`. The host had fed each output period back into
   the next envelope step, causing cumulative pitch drift.
3. `$170E2-$1712E` programs and disables Paula DMA on a new note, enables it on
   the following interrupt, and installs the loop state on the next. The mixer
   now preserves that one-interrupt attack delay instead of starting samples in
   the command-parsing interrupt.

All four music sequences remain independent and loop together after 24,960
50-Hz interrupts for the shipped song. The corrected 30-second stereo preview
is `music_preview_milestone83.wav`.

## Verification

- strict C99 core build with `-Wall -Wextra -Wpedantic -Werror`: pass;
- UndefinedBehaviorSanitizer core build: pass;
- SDL input/fullscreen regression and sanitized variant: pass;
- strict SDL host syntax build using the local SDL interface: pass;
- normal and sanitized 300-tick smoke runs for all 42 levels: pass;
- intro-frame, static-banner, protection-input, RNG-seed, Quiffy-bank,
  pitch-envelope, delay, DMA-start, four-channel, and PCM hash checks: pass.

Preview artifacts:

- `bullfrog_intro_milestone83.png` — 680x438;
- `gameplay_render_timing_milestone83.png` — 662x500;
- `music_waveform_milestone83.png` — 1200x500;
- `music_preview_milestone83.wav` — 30 seconds, 44.1 kHz, signed 16-bit stereo.
