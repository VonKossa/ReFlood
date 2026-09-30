# Binary-verified live cavern palette

The level renderer uses four bitplanes. The interactive gameplay loop begins at
`$A7BC` and passes palette pointer `$17F2C` to Copper-list builder `$EE4A` twice,
once for each display buffer (`$A7CA-$A7F0` and `$A7F4-$A81A`).

| Reg | Amiga | RGB888 |
|---:|---:|---:|
| COLOR00 | `$000` | `#000000` |
| COLOR01 | `$323` | `#332233` |
| COLOR02 | `$548` | `#554488` |
| COLOR03 | `$689` | `#668899` |
| COLOR04 | `$610` | `#661100` |
| COLOR05 | `$730` | `#773300` |
| COLOR06 | `$B60` | `#BB6600` |
| COLOR07 | `$C92` | `#CC9922` |
| COLOR08 | `$341` | `#334411` |
| COLOR09 | `$451` | `#445511` |
| COLOR10 | `$560` | `#556600` |
| COLOR11 | `$670` | `#667700` |
| COLOR12 | `$236` | `#223366` |
| COLOR13 | `$367` | `#336677` |
| COLOR14 | `$89A` | `#8899AA` |
| COLOR15 | `$CB9` | `#CCBB99` |

Four bitplanes select indices 0-15. The remaining words following `$17F2C`
belong to adjacent palette data and are copied by the generic 32-register
Copper builder, but cannot be selected by this four-plane display.

## Superseded setup-palette finding

`$17FEC` is genuinely passed to `$EE4A` at `$9792` and `$A004`, but those calls
prepare the setup/transition display. They do not survive entry into the live
gameplay loop, which rebuilds both Copper lists from `$17F2C`. `$B60E` remains
the separate five-plane intro/presentation palette (`BPLCON0=$5200`).

## Plane significance

No permutation is involved. Descriptor construction at `$B366/$B3EC` records
the start of each planar sprite record, while `$E4AA` consumes four consecutive
image blocks and blits them to the four consecutive display planes. `$EE4A`
programs those display-plane pointers in the same ascending order. Source plane
0 therefore supplies color-index bit 0, through source plane 3 supplying bit 3.
