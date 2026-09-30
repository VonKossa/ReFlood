# Flood Milestone 44 — mutable trigger dispatcher

## Corrected trigger-record meaning

The 10-byte record layout is:

| Offset | Meaning |
| ---: | --- |
| +0 | event/map ID |
| +2 | destination map offset |
| +4 | width |
| +6 | height |
| +8 | byte offset into the mutable 640-byte trigger payload |

The final word is not a literal tile or marker. `$12E12` exchanges each map
byte with a byte from the payload area, advancing payload contiguously while
the destination advances by rows of 128. Repeating an event can therefore
toggle a region and preserve the displaced map bytes.

## Original-disk recovery

Earlier milestone data retained only the `count*10` record prefix. The custom
disk directory was decoded at track 11: eight-byte names plus start track, end
track, and byte size. `MAP001.D`, `MAP005.D`, and following files contain up to
four packed level chunks each. The supplied decompressor format recovered all
42 full `$3490` chunks and their complete `$280` trigger payloads.

The recovery was independently validated: Level 1's reconstructed terrain,
16-byte header, and record prefix exactly equal the prior extracted files.

## Dispatcher integration

`flood_activate_trigger()` translates `$12DA6/$12E12/$12E48`:

- scans matching records from last to first;
- swaps rectangular map and payload bytes;
- sends newly exposed marker values below 26 through the marker initializer;
- supports Quiffy entering `$FF` (mode 0) or `$03` (mode 1) switch cells;
- dynamically creates known marker-19 Mines, marker-20 State-22 bolts, and
  marker-22 Quiffy relocation points.

Level 5 event 1677 proves multi-record dispatch: two records swap seven cells,
including six marker-20 payload bytes that create a vertical bank of six bolt
launchers.

## Validation and boundary

Strict C99 compilation with `-Wall -Wextra -Wpedantic -Werror` passes. Tests
cover full-payload loading, Level 1's marker-22 swap and reverse toggle, Level
5's two matching records and six dynamic bolts, and all prior mechanics.

The trigger core is exact, but several marker families initialized by `$E7B0`
remain placeholders. Completing those marker initializers is the clean next
step and may also expose the unresolved Sparkling Fungi/static-hazard path.
