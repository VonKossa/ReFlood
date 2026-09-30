# Flood Milestone 49 — State 1 Doctor Dusty

The runtime dispatcher selects `$13782` for State 1. Marker 1 follows the
special `$E7FE-$E836` initializer path and reserves two consecutive 20-byte
records. The first becomes Doctor Dusty with velocity `(2,4)` and sprite base
`$E2`; the second is initialized at the same position but left in State 0.

## Doctor Dusty (`$13782-$13858`)

Vertical velocity gains one each update. Animation uses the alternating
`$17E86` step and wraps through `$E2-$E5`. `$10D5E` applies the same swept
32×32 ledge-walker collision logic used by the State-12 Lumpy/Snail family.
The sprite is drawn at the old position, then motion is committed. A post-move
32×32 overlap with Quiffy's centered body removes 16 Life Force.

When the reserved next record is still State 0, Doctor Dusty changes it to
State 10, puts it at `(doctor_x+12,doctor_y)`, sets vertical velocity to zero,
and sets its byte-9 countdown to 20. Because the original dispatcher then
advances to that adjacent record, State 10 receives its first update in the
same frame. The host preserves this ordering.

## State-10 companion (`$1385A-$13922`)

The countdown is decremented before use. While non-negative, vertical velocity
decreases or increases according to record byte/word `+8`, clamps to `-8..+8`,
and switches to the increasing phase at the upper clamp. A swept 1×8 probe at
`(x+8,y+12)` cancels blocked vertical motion. The original 16×16 frames
`$28-$2B` are selected from the countdown low bits and drawn at `(x+4,y+4)`.

When the countdown passes below zero, the object moves up 12 pixels, becomes
State 9, clears animation, and does not draw during that transition update.

## State-9 burst (`$10A10-$10A90`)

The burst requests sound effect 7 on its first update. Six visible updates use
the doubled frame sequence `$98,$99,$99,$9A,$9A,$9B`; the first frame alone
removes 40 Life Force on 32×32 overlap. The seventh update returns the reserved
record to State 0. The SDL host records the sound request for a future mixer;
no audio backend is introduced in this milestone.

Tests cover paired marker allocation, same-pass State-10 activation, shared
walker collision, Doctor damage, companion collision and velocity clamps, the
no-draw transition, burst timing, sound request, burst damage, and cleanup.
