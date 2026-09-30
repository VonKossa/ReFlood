# Flood reverse engineering - Milestone 18

## Scope

This pass mapped the central collectible/map-item dispatch around `$FDA0-$10168` and connected it to the already recovered timed power-up and flood-control code.

## Named pickup IDs

Seven useful item IDs are now high confidence:

- `E1` Stout -> extra life (`$17E76++`).
- `E2` Cocktail -> `$E1CE = 50`. `$E1CE` is the invulnerability/grace timer: while nonzero the player Life Force / oxygen are protected/restored by the surrounding player code.
- `E3` Orange Can -> `$E1E8 = 50`. The update loop turns this into `$E1F4`, which suppresses normal interaction/update paths; this matches the manual's creature-disappearing / butterfingered effect well enough to name the timer.
- `E4` Droplet -> flood speed 6.
- `E5` Plunger -> flood pause 100.
- `E7` Parachute -> 1000-tick parachute timer.
- `E8` Balloon -> 1000-tick balloon timer.

## Weapon exchange mechanism

The most important engine discovery in this pass is `$10106`.

Flood supports six weapon map IDs (`DD`, `DE`, `EA`, `EB`, `EC`, `ED`). They map to the internal selected-weapon codes stored in `$E1EE`:

```
DD -> 1
DE -> 4
EB -> 8
EC -> 10
ED -> 13
EA -> 16
```

When Quiffy touches a different weapon, the game first writes the *currently selected weapon* back into the map cell, then stores the new code in `$E1EE`. This is a true exchange model: Quiffy carries one selected weapon and leaves the previous one behind.

This is now represented in portable C as `flood_swap_weapon()`.

## Items deliberately left provisional

`EE`, `FF`, and `03` have distinct behavior but are not yet named. `FF/03` feed opposite mode values into `$12DA6`, which is consistent with a switch/trigger pair, but `$12DA6` is complex enough that it should be traced before assigning the label.

Hearts and Mines appear to live in active-object handlers rather than this simple map pickup switch. A nearby `$FB42` object heals 64 Life Force and adds score on overlap, making it a strong Heart candidate.

## Next target

Trace the six weapon fire/projectile paths keyed by `$E1EE`. That should map the six internal codes/map IDs to the manual's named hardware and recover ammunition/projectile behavior.
