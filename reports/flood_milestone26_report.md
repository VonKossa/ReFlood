# Flood reverse engineering — Milestone 26

## Main result: Doctor Dusty and Lumpy Wanderer corrected

A corrected full global 32x32 sprite atlas exposed a cast-mapping error from earlier milestones.

### Doctor Dusty = runtime state 1

The state-1 handler at `$13782` computes its rendered sprite as `$E2 + (animation & 3)`. The original recovered `$E2-$E5` frames clearly show the hard-hatted creature. This directly matches the manual's Doctor Dusty description.

The handler also initializes an auxiliary projectile payload in its extended record when idle. Separately, runtime state 10 is a small bouncing projectile using stick-like 16x16 graphics. Together these are strong code-and-art evidence for Doctor Dusty's dynamite behavior.

### Lumpy Wanderer = runtime state 12, subtype `$30`

Level marker 12 creates a shared state-12 creature with subtype `$30`. The shared renderer uses `$50 + subtype + phase`, selecting global sprites `$80-$87`. These recovered frames depict a floating one-eyed creature, strongly matching the manual's Lumpy Wanderer description (floating, wall/ceiling following, rotating wet eyes).

### Corrected shared-creature family

- `$30` -> Lumpy Wanderer -> `$80-$87`
- `$38` -> Psycho Teddy -> `$88-$8F`
- `$66` -> Snail -> `$B6-$BD`
- `$6E` -> Bulbous Headed Vong -> `$BE-$C5`

## Sparkling Fungi

The previous state-22 theory remains withdrawn. State 22 is a horizontal projectile/emitter trap. Sparkling Fungi is stationary and instantly lethal according to the manual, and no remaining ordinary roaming-object handler currently matches that combination. The next static search should therefore treat Fungi as a likely map/static hazard rather than assuming it lives in the normal runtime-creature dispatcher.

## State 23

Static search has reached diminishing returns. We now have an exact debugger plan to catch the first write of behavior value 23 into the runtime table. A value-matched WinUAE watchpoint can identify the creator regardless of whether the value is produced by a literal write, copy, or arithmetic transition.

See `state23_winuae_watchpoint.md`.

## Next target

1. Trace special tile/static-hazard contact logic to locate Sparkling Fungi.
2. Run the state-23 value watchpoint in WinUAE and capture the creator PC/object record.
3. Once the creator is known, identify the three-hit object from its subtype/sprite/map context.
