# Flood Milestone 68: exact Quiffy item dispatcher

## Result

Milestone 68 translates the two original map-interaction paths at
`$FC76-$FD26` and `$FDA0-$10168`.  The Milestone-67 host handled weapon
selection, `$E4/$E5`, `$E7/$E8`, and switches; it omitted the ordinary
collectibles, three major power-ups, mechanism controls, exit, score object,
paired transport family, and `$DC` auxiliary-record activation.

The two routines have independent map-cell caches.  This matters for the
persistent `$E7/$E8` tiles and for `$DC`: a tile reacts only on cell entry, but
can react again after Quiffy leaves and returns.

## Recovered interaction table

| Tile | Exact core result | Removed |
| ---: | --- | :---: |
| `$24-$27`, `$94` | remaining count -1, score +2, sound 16 | yes |
| `$88` | when remaining count is zero, write -1 and sound 60 | no |
| `$DB` | no side effect | yes |
| `$DC` | auxiliary State 1, clear record motion, remove tile, Quiffy Y -16 | yes |
| `$DD/$DE/$EB/$EC/$ED/$EA` | select weapon state 1/4/8/10/13/16 and restore the old weapon tile | swap |
| `$DF/$E0` | pause/resume States 16-18, sound 16 | yes |
| `$E1` | lives +1, sound 58 | yes |
| `$E2` | score +5, protection counter 50, sound 16 | yes |
| `$E3` | Orange counter 50, sound 16 | yes |
| `$E4` | flood speed 6, sound 10 | yes |
| `$E5` | flood pause 100 | yes |
| `$E7/$E8` | parachute/balloon 1000, mutually exclusive, sounds 38/37 | no |
| `$EE` | score +20, sound 54 and original presentation call | no |
| `$FF/$03` | trigger modes 0/1, sounds 26/63; `$03` clears first | no/yes |
| `$9C-$AF` | paired transport and sound 51 | no |

Cocktail handling also now follows `$D07C-$D0BA`: each active visit decrements
the counter and restores Life Force to 511 and air to 63.  It is consequently
more than a fatal-terrain flag.

Orange Can follows `$CB16` and `$CDA4-$CDC4`.  Its latch skips the ordinary
item, weapon, action, and object dispatcher—including drawing—while Quiffy,
Aunt Matilda, HUD, and flood processing continue.  The latch produces the
original final skipped visit after the timer reaches zero.

## Paired transports

`$9C-$A5` pair with `$A6-$AF` by adding 10; the upper family subtracts 10.
`$10168` searches backward from map offset `$31FF`, so the last matching cell
wins.  Destination X uses Quiffy's orientation (`-20` or `+12` pixels) and Y
is offset by -8, or -24 after the `$DC` entry path.  A ten-step transition
teleports when its counter is 6.

## Original-level audit

The 42 static maps contain 787 required collectible cells, 42 exits, 28 extra
lives, 142 Cocktails, 13 Orange Cans, 90 flood accelerators, 20 flood pauses,
11 parachutes, 110 balloons, 76 `$DB` cells, five `$DC` cells, eight `$EE`
cells, 67 weapons, and 94 paired-transport endpoints.

## Verification and boundary

Focused tests cover every dispatcher family, both independent caches,
Cocktail gauge restoration, the Orange skip latch, mechanism control, exit
arming, reverse-order partner selection, orientation/Y offsets, and the
counter-6 teleport.  Strict C99, UBSan, and normal/UBSan 42-level sweeps pass.

The item dispatcher itself is now complete at the reconstruction's core-model
level.  The `$DC` handoff is exact, but its following auxiliary State-1/2
movement and sprite sequence remains a separate bounded subsystem.  `$EE`'s
presentation-string call and the transport fade are not yet drawn by SDL.
Automatic level loading after `$88` is also intentionally left for the level
progression milestone.
