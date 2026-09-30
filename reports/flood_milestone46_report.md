# Flood Milestone 46 — live cavern palette correction

The previous palette conclusion was one display transition too early.

`$A004` does call the four-plane Copper builder `$EE4A` with `$17FEC`, but the
interactive cavern loop starts at `$A7BC`. At loop entry, `$A7CA-$A7F0` and
`$A7F4-$A81A` rebuild the two Copper lists with palette pointer `$17F2C`.
Consequently the grayscale/orange/purple `$17FEC` table is not the palette seen
during live play.

The first 16 words at `$17F2C` are:

`000 323 548 689 610 730 B60 C92 341 451 560 670 236 367 89A CB9`

These are now used by the SDL renderer and every generated diagnostic preview.

## Bitplane order

The earlier natural-order result remains valid. Sprite descriptor constructors
at `$B366` and `$B3EC` store the unadjusted record base. The `$E4AA` blitter
walks four consecutive source blocks while advancing through display planes
separated by `$2940`; `$EE4A` programs BPL1 through BPL4 in that same ascending
memory order. Thus source blocks map to index bits 0, 1, 2, and 3 respectively.

## Corrected interpretation

| Address | Role |
| --- | --- |
| `$17F2C` | live four-plane cavern palette |
| `$17FEC` | four-plane setup/transition palette |
| `$1802C` | five-plane menu/high-score palette |
| `$B60E` | separate five-plane presentation palette |

This closes the palette/bitplane-order problem before further enemy-state work.
