# Flood Milestone 45 — complete marker initializer table

## `$E7B0` jump table

All 26 entries are decoded and routed:

| Marker | Initializer result |
| ---: | --- |
| 0 | no operation |
| 1 | State 1 Doctor Dusty; +4=2, +6=4, subtype `$E2`; map cleared |
| 2 | State 12 Snail; subtype `$66`, +4=2, flag=4; map cleared |
| 3 | zeroes all 128 packed bytes of tile-3 graphics; map retained |
| 4 | map cleared |
| 5 | State 5 Beady Ball; +4=4, +6=16; map cleared |
| 6,7 | map cleared |
| 8 | State 8 Vacuous Gombo; map cleared |
| 9,10 | map cleared |
| 11 | State 11, subtype `$4E`, origin saved, flag=3; map becomes `$CB` |
| 12 | State 12 Lumpy Wanderer; subtype `$30`, +4=2, flag=4 |
| 13 | State 13; map cleared |
| 14 | State 14 Plonkin Donkin; +4=4, +6=16; map cleared |
| 15 | map cleared |
| 16 | State 16, subtype `$4E`, saved origin, random timer 1–100; map `$C9` |
| 17 | State 17, subtype `$4E`, saved origin, random timer 1–100; map `$CD` |
| 18 | State 18, subtype `$4E`, saved origin, flag=3; map `$CB` |
| 19 | State 19 Mine; map `$E6` |
| 20 | State 22 bolt; saved origin, sprite base `$14`; map cleared |
| 21 | enables water, writes water-state 4, saves source coordinates |
| 22 | sets Quiffy coordinates and clears phase/orientation; map cleared |
| 23 | configures clamped bounds and mechanism flag; map cleared |
| 24 | State 12 Vong; subtype `$6E`, +4=4, flag=4 |
| 25 | State 12 Psycho Teddy; subtype `$38`, +4=4, flag=0 |

The original 16-bit generator at `$D77C` is also integrated:
`state = state * $24A1 + $24DF` modulo 65536. States 16/17 use its remainder
modulo 100 plus one, reproducing their initialization timer exactly.

## Population validation

Loading all 42 original maps through the initializer produces the expected
static totals: State 1=6, State 8=80, shared State 12=375, State 14=88,
States 16/17/18=89/62/8, Mines=109, and bolts=20. Tests also verify every
initializer's active state, subtype, motion/origin fields, flags, map result,
tile-3 blanking, water seed, Quiffy relocation, bounds, and RNG output.

## Accuracy boundary

Initialization is now complete, but most enemy/mechanism update handlers are
not yet integrated. Marker 11 remains dormant: it has zero static placements
and no recovered trigger payload creates it. Nothing in this pass identifies
it as Sparkling Fungi. The best next target is the shared State-12 handler,
because one implementation activates 375 Snail/Lumpy/Teddy/Vong objects and
adds the largest amount of visible gameplay.
