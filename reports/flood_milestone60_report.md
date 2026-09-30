# Flood reverse engineering — Milestone 60

## Result

Milestone 60 closes the weapon-system accuracy boundary left by Milestone 59.
The grenade's collision, bounce, twin-lobe blast, damage rectangles, terrain
probes, and clipping behavior now follow `$D678-$D77A` and
`$126F2-$1293C`. The boomerang's corridor acquisition, launch, target tracking,
steering, impact, and lifetime now follow `$104FA-$10812`. The shared `$F9E6`
object eligibility table and inclusive collision comparisons are translated as
part of the same work.

## Grenade State 2: bounce correction

State 2 starts by adding 2 to DY and clamps it to `-12..16`. It calls the swept
collision routine with the centered 8x8 body at `(X+4,Y+4)`.

- A horizontal solid reverses DX and requests sound 17.
- An upward vertical solid simply reverses DY.
- A downward vertical solid halves DY, reduces the magnitude of DX by one when
  the halved DY is not greater than that magnitude, then reverses DY and sets
  word `+$10` to 1.
- Timer expiry changes to State 3, calls `$126F2`, resets animation, and skips
  movement and drawing on that update.

The horizontal-friction step was absent from the Milestone-59 behavioral
translation and is now regression-tested for the combined X/Y collision case.

## Grenade State 3: exact twin lobes

`$126F2` aligns the impact X downward to 16 pixels and subtracts 16 from Y.
It stores two anchors at `aligned X + 32` and `aligned X - 32`, copies the same
Y to both, sets timer 7, clears the two animation words, and requests sound 6.

Every visit to `$127E8` executes four iterations with phase register D5
descending from 3 to 0:

| Lobe | First X | Step | Quiffy rectangle | Sprite expression |
| --- | ---: | ---: | ---: | --- |
| Right (`+$00/+$02`) | aligned X + 32 | -16 | 28x28 | `$98 + anim + D5` |
| Left (`+$04/+$06`) | aligned X - 32 | +16 | 32x32 | `$98 + aux + D5` |

Each overlapping rectangle subtracts 10 Life Force, so overlap at multiple
positions produces multiple hits. A sprite is drawn only while its expression
is at most `$9B`. The original tests the right-lobe X before *both* draws; the
host preserves this apparent clipping bug rather than correcting it.

After drawing, two independent 80x32 `$F9E6` queries start at the right anchor
and at `left anchor - 48`. The blast can therefore advance two objects and
award 10 points for each.

The right/left contraction logic uses horizontal swept probes with a 32x1
extent:

- left probe: `(left+32,leftY+8)`, delta `(+16,0)`;
- right probe: `(right-16,rightY+8)`, delta `(-16,0)`.

An open edge moves that anchor 16 pixels inward. A solid edge instead advances
only that lobe's animation word by global alternating step `$17E86`. The timer
then decrements; values at or below zero clear the action state.

## Boomerang State 5: corridor acquisition

After its normal horizontal collision/movement attempt, State 5 sets target
word `+$10` to `-1`, restores timer 20, and advances to State 6. `$10792` then
performs two vertical searches at the boomerang's X:

1. Starting at displacement zero, add 16 until a 16x16 swept query reports a
   solid Y edge; save `Y + displacement` as the bottom.
2. Starting at zero, subtract 16 until solid; save that coordinate as the top.
3. Query `$F9E6` over `(X,top,16,bottom-top)`.

The returned object index is stored only when it is strictly greater than
zero. Index zero is therefore a real result that State 5 intentionally rejects.

## Boomerang States 6-7: launch and homing

State 6 returns the action to Quiffy's `(X,Y+8)`. It compares that point with
the shared target-coordinate words at `$1295A/$1295C`, producing signed X and Y
directions. X is scaled by 16, Y by 4, and the initial X velocity is also saved
as the desired velocity in `+$0C`. It enters State 7 with timer 20 and sound 24.

State 7 treats a signed target word greater than zero as an object index. It
refreshes the shared target coordinates from that object each update. If the
object state has become zero, the action terminates at the object's coordinates.
For an untargeted action, the target point is Quiffy's Y and
`Quiffy X + (horizontal bank - 2) * 128`; only this branch decrements the
20-update timer.

Steering retains a subtle one-update lag:

- when actual DX equals desired DX, the new signed target direction is saved
  as desired DX and signed target Y immediately becomes DY, but the old DX is
  still used for the current movement;
- otherwise actual DX moves toward desired DX by exactly 2;
- after movement, `$F9E6` checks a 16x16 query; impact advances the object,
  awards 10 points, stores the impact position, and clears the action;
- non-impact X positions at or below zero clear the action without drawing;
- otherwise animation advances modulo eight and draws `$20-$27`.

The unused wrapper at `$1293E` writes Quiffy's coordinates into the shared
target words, but an exhaustive direct-call and relative-branch reference scan
of the resident image found no caller. The host therefore retains the words as
explicit persistent `FloodGame` scratch state rather than silently replacing
their initial contents with a guessed target.

## Shared `$F9E6` filter

The 26-entry state table admits only States 1, 5, 8, 12, 14, 23, 24, and 25.
All other object states are skipped. X and Y checks use inclusive comparisons
against the query extent and the object's 32x32 host collision extent. The
first eligible overlap is returned; no overlap returns `-1`.

## Verification

Focused regressions cover:

- the eight exact initial lobe sprite positions and phase order;
- repeated Quiffy damage across lobe positions;
- both independent 80x32 object hits;
- open-edge anchor contraction and seven-update lifetime;
- combined wall/floor bounce and horizontal friction;
- full-map vertical corridor acquisition;
- strict target-index-greater-than-zero behavior;
- launch velocity, delayed desired-X change, two-unit steering, and target
  coordinate refresh.

The complete suite is built in strict C99 mode with `-Wall -Wextra
-Wpedantic -Werror`, then repeated under UBSan. Both the normal and instrumented
headless hosts are run for all 42 decoded levels.

## Next boundary

The weapon/action dispatcher and its previously approximate grenade and
boomerang paths are now complete at the reconstruction's object-model level.
The next useful milestone is a systematic audit of the remaining global status
and HUD paths—weapon indicator, Life Force/air presentation, score/lives, and
their exact update ordering—rather than another weapon-state rewrite.
