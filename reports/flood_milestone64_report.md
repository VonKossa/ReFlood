# Flood Milestone 64: exact tile overfetch compositor

## Result

Milestone 64 replaces the SDL renderer's direct world-pixel sampling with the
original map compositor's tile traversal: 22 columns by 14 rows, coarse camera
selection, fine-phase cropping, and the three animated tile aliases performed
immediately before graphics lookup.

The output remains the exact 320x208 visible window established in Milestone
63. The extra tile on each axis supplies the pixels exposed by camera phases
1-15 and is clipped away after composition.

## Coarse camera selection

At `$A850-$A878`, the frame loop snapshots camera X/Y from `$17E82/$17E84`
and constructs the tilemap offset:

```text
coarse_index = (camera_y >> 4) * 128 + (camera_x >> 4)
```

That offset is added to the level map base before the compositor call. The
independent low nibbles are the fine phases:

```text
phase_x = camera_x & 15
phase_y = camera_y & 15
```

Milestone 62 already reconstructs the complementary horizontal `BPLCON1`
delay. For a chunky visible-output model, both axes reduce to placing the
coarse tile origin at `(-phase_x,-phase_y)`.

## Exact traversal at `$EC4E-$ED64`

The normal terrain path initializes its outer counter to 13 and inner counter
to 21. Because both are `DBRA` loops, this produces exactly 14 rows and 22
columns.

For every tile:

1. Read one map byte.
2. Apply any animated alias.
3. Multiply the resulting tile number by 128 (`LSL.L #7`) to locate its
   four-plane 16x16 record.
4. Blit one 16-pixel word across 16 scanlines (`BLTSIZE=$0401`) to each of the
   four planes.
5. Advance the horizontal destination by two bytes.

After 22 tiles the routine adds `$0294` to its running destination offset.
Together with the 44 bytes consumed by the row, that is `$02C0` = 704 bytes,
or exactly 16 scanlines at the 44-byte backing stride.

The host implements the same traversal and copies decoded tile pixels into a
320x208 crop. It no longer chooses a tile independently for every output
pixel.

## Animated tile aliases

`$ECB4-$ECF2` modifies three logical map values before tile-record lookup:

| Map byte | Graphics tile used |
| ---: | --- |
| `$1D` | `$1D + $17E88` |
| `$20` | `$1D + (3 - $17E88)` |
| `$47` | `$47 + $17E86` |

`$17E88` increments modulo four at `$B2CE-$B2DA`. `$17E86` alternates between
zero and one at `$B2B6-$B2CE` as the two display buffers exchange roles. The
host snapshots equivalent render phases at the start of each gameplay tick so
the subsequently presented frame uses the values with which it was built.

The `$1D` and `$20` families therefore traverse the same four graphics records
in opposite directions, while `$47` alternates between `$47/$48`.

## Edge behavior

Default camera maxima are `(1728,1392)`. Adding the 320x208 visible extent
reaches exactly `(2048,1600)`, the exclusive map boundary. Consequently every
visible sample is always backed by a valid map cell.

The 22x14 traversal intentionally continues into invisible overfetch. Near the
full-map right and bottom edges, some of those speculative tile reads would be
beyond the decoded 128x100 map. Their contents cannot enter the visible crop.
The host skips those reads and leaves the guard pixels at palette index zero,
making the safety policy explicit without changing visible output.

## Verification

Regression coverage includes:

- all three animated aliases and an unchanged ordinary tile;
- a camera with phases `(1,15)`, checking all four visible corners against
  their expected coarse source tiles;
- the full-map maximum camera, proving all 66,560 visible pixels remain valid;
- the Milestone 62 sixteen-phase HUD placement checks;
- exact camera defaults and marker-derived bounds;
- every pre-existing gameplay reconstruction test.

The strict C99 core test and complete 42-level standalone sweep are repeated
under undefined-behaviour sanitization. The preview generator independently
implements the 22x14 traversal and shows phases `(0,0)`, `(7,9)`, and
`(15,15)` with the tile grid overlaid.

## Next accuracy target

Milestone 65 should reconstruct the second terrain path at `$ED66-$EE48`,
which draws the dynamic overlay/water tile layer into only two playfield
planes. This will replace the remaining direct water representation with the
original layered composition order and tile-zero skip behavior.
