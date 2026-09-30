# Flood Milestone 87: deep-water oxygen and audio-envelope correction

## Result

Milestone 87 fixes oxygen consumption in settled water, restores the small
centred Bullfrog presentation, and makes both notes of the boomerang effect
audible through an instruction-level envelope correction.

## Exact water-depth lookup

`$109A6` indexes the byte table beginning at `$109E6`. The table has 42
entries, corresponding to every secondary-water state `$00-$29`:

`00 00 00 00 01 00 00 00 01 01 01 00 02 02 02 00`

`04 04 04 00 06 06 06 00 08 08 08 00 0A 0A 0A 00`

`0C 0C 0C 00 0E 0E 0E 00 10 00`

The old host array had only 40 entries and most groups were shifted two states
early. Most importantly, settled state 40 fell outside the array and returned
zero. `$DEB4-$DEF2` therefore interpreted visually full static water as dry.
The corrected table maps state 40 to depth 16, so air decreases on the original
alternating `$17E86` phase and drowning damage begins after air becomes
negative. A regression checks all 42 state-to-depth mappings.

## Bullfrog presentation canvas

The animation compositor still works in the right-hand 160x96 region of the
320-pixel `BIG_PIC` backing. The error was making those dimensions the SDL
logical screen, which enlarged the artwork to fill the window. Copper
`DIWSTRT=$74CC` and `DIWSTOP=$D45C` establish the small 96-line presentation
window. The host now renders a 320x240 black PAL canvas and places the native
work image at `(80,72)`. It is centred, remains undistorted, and uses the same
logical stage size as the following title screen.

## Two-note boomerang effect

The action routine `$104FA-$10546` correctly requests only sound 24. Its track
contains two notes: the first uses volume macro `$C7`, and the higher second
uses `$C8`. No extra guessed sound dispatch is required.

The envelope routine applies a segment delta when flag bit 0 is set at
`$17178-$17188`. After each duration/repeat step, both the not-yet-complete path
and the next-segment path reach `$17200`, setting that flag again. The host had
set it only when the entire repeat count completed. Consequently `$C7` applied
its `+1` once and held the first boomerang note at volume 1 for roughly 1.8
seconds. It now ramps from 1 to 63 as the original does; `$C8` then starts the
second note at full volume and fades it to zero.

The correction is deliberately shared by effects and music because `$1716A`
is their common envelope interpreter. Deterministic PCM hashes and an explicit
sound-24 two-note phase test protect the corrected behavior.

## Verification

- all 42 exact water-state lookup entries and settled-state oxygen drain: pass;
- centred 320x240 intro bounds and all six retained content hashes: pass;
- sound-24 first-note ramp, second-note transition, and deterministic effect
  and music PCM hashes: pass;
- strict C99 core and SDL-interface builds with warnings as errors: pass;
- core and SDL-interface tests under UndefinedBehaviorSanitizer: pass;
- normal and sanitized 300-tick smoke runs for all 42 caverns: pass.

Artifacts: `oxygen_meter_milestone87.png`, `bullfrog_intro_milestone87.png`,
`boomerang_sound_milestone87.png`, and `boomerang_sound_milestone87.wav`.
