# Flood Milestone 92: presentation and drowning regression correction

## Result

Milestone 92 fixes unreliable fullscreen round trips and stale presentation
pixels, centres the visible Bullfrog artwork, restores instruments that vanished
later in the intro music, and keeps exhausted HUD gauges empty during drowning.

## Fullscreen state and buffers

The host previously toggled a private Boolean and reissued desktop fullscreen
on every presentation-size change. A backend could accept the first transition
while the private and actual states later diverged; leaving fullscreen also did
not restore the current stage's window geometry. Old pixels could remain in a
back buffer during the intro.

The F-key path now reads `SDL_GetWindowFlags`, performs one requested mode
transition, restores the current stage at 2x size and centres the window on
exit, reapplies its logical size, and presents two black clears. Logical stage
changes retain fullscreen without submitting another mode request. SDL stub
tests cover both directions, repeat suppression, stale cached state, geometry
restoration, and buffer clears.

## Visible Bullfrog centring

An exhaustive frame scan confirms the 160-pixel source work buffer has visible
pixels only at relative X=0..134; relative X=135..159 is permanent black
padding. The former X=80 placement therefore put the visible image twelve
pixels left of centre. The compositor now starts at X=92. Visible bounds are
X=92..226, giving integer margins of 92 and 93 pixels.

## Per-note audio-envelope restart

The original 68000 note path at `$1733E-$17380` clears phase, value, base,
repeat and tick state for both channel envelopes, sets the apply-delta bit, and
clears the finished bit. It leaves the selected macro pointer and loop mode
unchanged. The reconstruction previously restarted only sample DMA.

Once a volume macro completed at zero, later notes could therefore remain
silent until another macro command happened to arrive. During the now-correctly
timed full intro this muted most of the mix for extended passages. The restored
per-note restart keeps all four channels audible at the 30-second checkpoint.
The sound-effect suite, including both boomerang notes, still passes with the
same shared interpreter.

## Exhausted gauges and drowning rate

The HUD begins from a template whose air and Life Force gauge columns contain
filled pixels. The former negative-counter branches skipped gauge drawing and
revealed those filled template bytes. Both gauges are now always rebuilt, with
zero used for non-positive counters.

The damage routine itself was already faithful and is retained. At depth eight
or greater, only display-buffer phase 1 decrements air. A result below zero
subtracts four Life Force on that same update. Ten 60 ms gameplay updates thus
contain five damage phases: air 0 becomes -5 and Life Force 100 becomes 80.
From full Life Force 511, 128 damage phases take 15.36 seconds to reach -1 after
oxygen is exhausted.

## Verification

- strict C99 core and SDL-interface builds with warnings as errors: pass;
- core and SDL-interface tests under UndefinedBehaviorSanitizer: pass;
- intro artwork bounds and all retained animation hashes: pass;
- 30-second four-channel music state and five-second PCM hash: pass;
- negative air/Life Force HUD and ten-tick drowning cadence: pass;
- all 42 cavern headless smoke runs: pass.
