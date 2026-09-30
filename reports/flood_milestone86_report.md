# Flood Milestone 86: protection blink and corrected transitions

## Result

Milestone 86 resolves the five reported runtime discrepancies: Quiffy blinks
and is protected after level entry and respawn, carried balloon and parachute
graphics render, teleport mosaics retain their original colours, and the next
level waits for the completion effect to finish.

## Initial and respawn protection

The draw routine at `$D07C-$D0C2` tests protection byte `$E1CE`, decrements it,
restores life to 511 and air to 63, and skips Quiffy's draw when alternating
global byte `$17E86` is zero. The host already initialized `$E1CE` to 100 at
both level entry and respawn, but had implemented only the mechanical damage
protection. It now latches the pre-decrement protected state and renders the
player/weapon only on the matching alternating buffer phase. This retains the
correct final blinking frame when the timer changes from one to zero.

## Balloon and parachute

`$D01C-$D076` decrements the carried-item timer and draws global sprite `$C6`
for the parachute or `$C7` for the balloon at `(x,y-24)`. This occurs before
the protection draw gate. The SDL compositor now follows that order, including
the original behavior in which the carried item remains visible while Quiffy
is on a hidden blink phase.

## Full-colour transport mosaic

The earlier Milestone 78 claim that the framebuffer used interleaved bitplane
rows was wrong. Copper setup `$EE4A` adds `$2940` between successive plane
pointers, exactly 44 bytes by 240 scanlines. `$13FC4-$14198` applies each
stage's mask and vertical copy to all four separate planes. Therefore stages
1-5 preserve all four colour bits and produce full-colour blocks of 2x2, 4x4,
8x8, 16x16, and 32x32 pixels. The host renderer and regression expectations
have been corrected accordingly; hidden left-overfetch sampling at stage 5 is
unchanged.

## Completion-sound handoff

Exit effect 60 occupies all four effect channels. After the existing cavern
fade, the SDL loop now waits until every channel's tracker sequence has ended,
while continuing to service quit and fullscreen input. Only then does it call
the next-level loader and begin the next banner. Audio callback state is read
under the SDL device lock.

## Verification

- protection pre-decrement and post-respawn visible/hidden phases: covered;
- corrected full-colour geometry for all five mosaic stages: covered;
- effect 60 remains playing initially and reaches a finite idle state: covered;
- strict C99 core and SDL-interface builds with warnings as errors: pass;
- core and SDL-interface tests under UndefinedBehaviorSanitizer: pass;
- normal and sanitized 300-tick smoke runs for all 42 caverns: pass.
