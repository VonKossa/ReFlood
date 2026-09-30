# Flood reverse engineering — Milestone 23

## Summary

This milestone resolves two of the previously unnamed hostile runtime states and uses the original sprite payload as independent evidence.

### Vacuous Gombo = runtime state 8

State 8 (`$10834`) is now identified with high confidence as the **Vacuous Gombo**. The code shows a 32x24 hostile creature that chooses a fresh 8-way direction when stalled, moves at speed 4, cycles four frames `$90-$93`, and drains 8 Life Force on contact.

The decisive evidence comes from reconstructing raw entries `$90-$93` from `SPR_32.B`. The descriptor-builder at `$B360` proves that this file contains 150 consecutive 32x32 sprites, each `$280` bytes (four bitplanes plus mask). The recovered frames visibly show the Gombo's huge stomping boots, matching the manual's distinctive description.

Combat linkage is therefore exact:

`Vacuous Gombo: state 8 -> hit -> state 9 -> $10A10 explosion/removal`

### Plonkin Donkin = runtime state 14

State 14 (`$10C46`) is now a high-confidence **Plonkin Donkin** mapping. Its handler is dominated by vertical oscillation and repeated bounce/leap resolution: vertical velocity is driven between limits, horizontal velocity reflects from walls, downward terrain contact produces an upward reversal, and player contact removes 8 Life Force.

This is the best behavioral match for the manual's rarely seen, sleepy creature whose awakened behavior is mainly leaping about.

Combat linkage:

`Plonkin Donkin: state 14 -> hit -> state 15 -> $10A10 explosion/removal`

### Probable Snail: state-12 subtype $66

Level marker 2 creates state 12, subtype `$66`, with a low initial horizontal speed (`dx=2`). It uses the shared terrain-following creature handler and damages Quiffy on overlap. With Gombo now independently identified as state 8, this slow crawler is the strongest remaining match for **Snail**, but the name is kept provisional until its compressed 16x16 art is decoded.

### Three-hit destructible remains indirect

The special `23 -> 24 -> 25 -> 26` weapon target remains unresolved, but its creation path is narrower. There is no literal state-23 assignment in `RUN_PROG`, and `$EC0A` is not used dynamically outside the level initializer. State 23 must therefore arise through a computed/copied transformation or a secondary object mechanism.

## Updated known hostile cast

- state 1: Lumpy Wanderer (high-confidence behavioral inference)
- state 5: Beady Ball (high confidence)
- state 8: **Vacuous Gombo (now high confidence; sprite-confirmed)**
- state 12 / subtype `$38`: Psycho Teddy
- state 12 / subtype `$6E`: Bulbous Headed Vong
- state 12 / subtype `$66`: **probable Snail**
- state 14: **Plonkin Donkin (now high confidence)**
- state 22: probable Sparkling Fungi

## Next target

Decode the compressed `SPR_16.B` payload or the loader/decompressor feeding its 80 descriptor entries. That should visually confirm subtype `$66`, state 14, state 22, and several remaining state-12 variants in one pass. In parallel, trace the secondary object/update structures that can create the non-literal state-23 destructible.
