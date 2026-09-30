# Flood reverse engineering — Milestone 58

## Result

The SDL host now implements the original two-mode Quiffy death lifecycle. A
fatal hit no longer deducts a life and restores the player immediately. Quiffy
first enters a falling death state, then changes to the eight-frame cross on
landing; health, air, protection, and the life count are updated only when the
cross counter completes.

## Entry paths

The binary has two distinct entrances to mode 1 (`$E1D4 = 1`):

| Address | Cause | Additional action |
| ---: | --- | --- |
| `$D830-$D840` | Life Force is zero or negative | Set Aunt Matilda history hold `$E1B4` to 20. |
| `$D87E-$D894` | Fatal-terrain contact bit 8 | Enter mode 1 directly, with no hold write. |

The zero boundary matters: `tst.w` followed by `bgt` means exactly zero is
already fatal. The host now uses that same boundary.

## Mode 1: fall to support

Mode 1 continues through Quiffy's movement and collision path. At
`$DBE8-$DC28`, south-support contact bit 4 triggers the transition:

- subtract 8 from Quiffy's Y coordinate;
- reset the animation counter to zero;
- write death mode 2;
- request sound `$38`;
- leave the remainder of the movement routine immediately.

Parachute and balloon timers are cleared whenever a death mode is rendered.
The compact host clears them at entry, which produces the same subsequent
state while keeping the lifecycle explicit.

## Mode 2: cross animation and recovery

The renderer implements mode 2 at `$CDF0-$CE7E`:

1. Read the animation counter.
2. If its pre-step value is 8, request sound `$38`.
3. Add the global `$17E86` animation step, which alternates `1,0` between
   gameplay frames.
4. If the result is 16, finish the death without drawing another cross frame.
5. Otherwise clamp values above 7 to 7, add phase base `$28`, and pass the
   result to the `$50`-based player renderer.

The resulting visible sprite is therefore `$78 + min(counter, 7)`. Frames
`$78-$7F` form the expanding cross; `$7F` remains visible for counters 8-15.

At counter 16 the original writes:

| State | Restored value |
| --- | ---: |
| Death mode | 0 |
| Animation counter | 0 |
| Life Force | 511 |
| Air | 63 |
| Post-death field `$E1CC` | 8 |
| Cocktail protection `$E1CE` | 100 |

It then subtracts one from the lives word `$17E76`. If the result is not
positive, it raises the game-over state. The host mirrors this by stopping its
run loop after the last life completes.

`$E1CC` has no other direct absolute reference in the recovered resident
image, so Milestone 58 records its exact write but does not invent a consumer
for it.

## SDL integration

`FloodPlayer` now carries explicit `death_mode` and `death_phase` fields.
`flood_player_sprite_id()` returns the computed cross sprite during mode 2,
and the game tick advances recovery with the already recovered alternating
frame step. The former immediate life-loss/respawn block has been removed.

Regression tests cover:

- fatal terrain entering mode 1 without immediate life loss;
- cocktail protection bypassing fatal terrain;
- zero Life Force entering mode 1 and applying Matilda's hold;
- landing alignment, sound, and mode-1-to-mode-2 transition;
- all 16 counter-to-sprite results;
- the exact 31-update grounded sequence from initial entry to recovery;
- delayed life deduction, restored Life Force/air, 100-tick protection;
- final-life game over.

## Remaining boundary

The death state itself is complete. The next high-value target is the
weapon/action subsystem: its eight 18-byte records, dispatcher at `$CB40`, and
the exact Quiffy-to-weapon creation paths.
