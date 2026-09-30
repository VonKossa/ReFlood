# Flood reverse engineering — Milestone 27

## Main result: complete level-file and terrain-attribute layout

This pass moved the static-hazard search down one layer and recovered two core asset formats that the C reconstruction will need anyway.

### `MAPxxx.D`

Every `MAPxxx.D` file is a concatenation of compressed level chunks. The filenames are grouped by starting level (`MAP001.D`, `MAP005.D`, ...); the final `MAP041.D` contains only levels 41 and 42. In total the disk contains exactly 42 chunks.

Each chunk decompresses to exactly `$3490` (13,456) bytes and is laid out as:

- `$0000-$31FF`: 12,800-byte terrain tile map = **128 x 100 bytes**.
- `$3200-$320F`: 16-byte level control area.
- `$3210-$348F`: 640-byte trigger/logic table = **64 records x 10 bytes maximum**.

The pointer setup at `$9902` independently proves these offsets:

- `$17F0A = map_base`
- `$17F0E = map_base + $3200`
- `$17F12 = map_base + $3210`

The switch/trigger routine `$12DA6` reads a big-endian word at control offset `+2` as the record count and consumes 10-byte records as five words. The provided decoder exports all 42 tile maps, control blocks, active trigger records, and a JSON manifest.

### Level object markers

During initialization, `$E756` scans all `$3200` tile bytes and calls `$E7B0` on each. Tile values `0..25` are therefore special initialization markers embedded directly in the terrain map. `$E7B0` dispatches them into creature/object/setup creation and usually removes or replaces the marker.

This explains why searching only the runtime object table misses some level mechanisms: the original maps themselves contain the spawn/setup language.

## `BLOCKA/B/C.B`

All three block files use the same custom decompressor already recovered at `$1499C`. Each decompresses to exactly `$8100` bytes:

- `$0000-$7FFF`: 32,768 bytes = **256 tiles x 128 bytes**.
- `$8000-$80FF`: **256-byte tile attribute table**, one byte per tile ID.

The graphics size matches a 16x16, four-bitplane Amiga tile exactly: 16 rows x 2 bytes x 4 planes = 128 bytes.

The attribute-table placement is independently confirmed by runtime pointers:

- `$17F1E` points at the unpacked block graphics.
- `$17F1A = $17F1E + $8000`, exactly the 256-byte attribute table.

The recovered meanings remain:

- attribute bit 0 (`0x01`): prevents flood/water propagation;
- attribute bit 1 (`0x02`): blocking terrain for creature/player collision.

Other bits encode additional terrain behavior and are still being named.

## Sparkling Fungi search: what changed

A tempting false lead was the immediate-death path near `$D330`, because it compares the byte `$D9` and then writes `-1` to Life Force. This is **not a map tile check**. `$17F2A` is the raw CIA keyboard serial byte. `$D9` decodes to Amiga raw key `$13`, i.e. the restart key path; `$75` similarly decodes to ESC. This removes a misleading static-tile hypothesis.

The block attribute tables do not expose an obvious dedicated "instant death" bit used by the player collision path. That makes Sparkling Fungi more likely to be represented through a special object/contact mechanism than through a simple terrain attribute alone.

The rare state-16/17/18 family is also less attractive as a Fungi candidate: all three share the same subtype/art family and map tiles `$C9/$CB/$CD`, whose recovered graphics are clearly related mechanical/portal-style structures rather than fungi.

So Sparkling Fungi remains unresolved, but the search space is now considerably smaller and, crucially, the entire map/object-marker layer is directly inspectable.

## New tooling

- `decode_flood_levels.py` — unpacks all 42 levels and exports tile/control/trigger sections.
- `export_block_assets.py` — unpacks BLOCKA/B/C and exports graphics + per-tile attributes.
- `include_flood_level_format.h` — portable C layout/constants for the level data.
- `blocks/tile_attributes.csv` — all 768 bank/tile attribute entries.
- `levels/manifest.json` — all 42 levels with trigger counts, trigger words, and marker occurrence summaries.

## Next target

Use the decoded 128x100 maps and marker scan to identify the remaining special object classes by level frequency and spawn context, then trace the runtime contact handler that corresponds to the stationary instant-kill Sparkling Fungi. In parallel, the trigger records provide a new route for tracing indirect creation of special runtime states such as the state-23 destructible.
