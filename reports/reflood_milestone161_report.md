# ReFlood Milestone 161: multiplayer protection, death cross, and level cue

## Changes

- The other player's sprite now uses the current shared display-buffer phase for the same protection blink as the local player. This fixes Player 2 appearing continuously on Player 1's screen during level-entry protection (and the reverse case).
- The remote actor's mode-2 death-cross sprite is drawn instead of being suppressed. Player 2's cross uses the original sprite palette, matching Player 1's cross in both views. The other actor's original hurt bubble is drawn in the observer's view as well.
- Either Matilda's contact retains the original 10 Life Force damage and hurt-voice feedback for the affected actor, including a cross-player hit. The bubble now appears in both cameras.
- Every level banner explicitly schedules the protection-start voice when a surviving actor receives the first-frame protection update. The host triggers it when the 51-frame banner ends and gives the SDL audio callback one 1024-sample buffer interval (24 ms) before gameplay effects can replace those channels.

## Verification

- Strict C99 core and multiplayer suites: pass. The multiplayer contact test checks both cross hits, both hurt bubbles, and the pair of original hurt-voice IDs; two ghost overlaps still cause 20 total damage.
- SDL input/settings stub suite: pass. Pixel-level checks verify Player 2's alternate protected frame disappears in both cameras, the hurt bubble appears there, and the death-cross pixels match the original palette in both views.
- A two-banner SDL stub test loads levels 1 and 2 in sequence and verifies both players' 99-frame protection timers, 51×60 ms banner timing, the 24 ms audio lead, and active effect channels after the second cue.
- Strict real SDL2 build, UndefinedBehaviorSanitizer multiplayer suite, and a dummy video/audio session advancing from a synthetic level-1 exit into level 2: pass. A separate audio-mix check produced nonzero samples on both repeated sound-52 triggers.
- The source package is code only, with an empty `data/` directory and all ZIP entry timestamps normalized to 2025-01-01 to avoid clock-skew warnings.

## Observation and limits

The milestone 160 dummy-audio probe already showed sound 52 queued and both routed effect channels active at the end of the level-2 banner. The user's missing audible cue could not be reproduced on a physical audio device here. The explicit per-entry scheduling and one-buffer lead remove reliance on the shared sound queue and avoid immediate gameplay preemption, but physical listening remains to be confirmed. Other experimental-mode limits described in milestone 160 remain.
